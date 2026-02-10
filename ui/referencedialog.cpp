#include "referencedialog.h"
#include "ui_referencedialog.h"
#include "../easyposcore.h"
#include "../logging/logmanager.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QShortcut>
#include <QKeySequence>
#include <QMenu>

ReferenceDialog::ReferenceDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core,
                                 const QString &title)
    : QDialog(parent)
    , ui(new Ui::ReferenceDialog)
    , m_core(core)
{
    qDebug() << "ReferenceDialog::ReferenceDialog" << "title=" << title;
    ui->setupUi(this);
    setWindowTitle(title);
    
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget->setAlternatingRowColors(true);
    
    // Настройка горячих клавиш
    setupShortcuts();
    
    // Подключение сигналов
    connect(ui->searchEdit, &QLineEdit::textChanged, 
            this, &ReferenceDialog::on_searchEdit_textChanged);
    connect(ui->searchEdit, &QLineEdit::returnPressed,
            this, &ReferenceDialog::on_searchEdit_returnPressed);
    connect(ui->tableWidget, &QTableWidget::doubleClicked,
            this, &ReferenceDialog::on_tableWidget_doubleClicked);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested,
            this, &ReferenceDialog::on_tableWidget_customContextMenuRequested);
    
    // Устанавливаем фокус на поле поиска
    ui->searchEdit->setFocus();
}

ReferenceDialog::~ReferenceDialog()
{
    qDebug() << "ReferenceDialog::~ReferenceDialog";
    delete ui;
}

void ReferenceDialog::setupShortcuts()
{
    qDebug() << "ReferenceDialog::setupShortcuts";
    // Insert - добавить
    QShortcut *insertShortcut = new QShortcut(QKeySequence(Qt::Key_Insert), this);
    connect(insertShortcut, &QShortcut::activated, this, &ReferenceDialog::on_addButton_clicked);
    
    // Enter - редактировать
    QShortcut *enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(enterShortcut, &QShortcut::activated, this, &ReferenceDialog::on_editButton_clicked);
    
    // Delete - удалить
    QShortcut *deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(deleteShortcut, &QShortcut::activated, this, &ReferenceDialog::on_deleteButton_clicked);
    
    // F5 - обновить
    QShortcut *refreshShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(refreshShortcut, &QShortcut::activated, this, &ReferenceDialog::on_refreshButton_clicked);
    
    // Esc - закрыть
    QShortcut *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &ReferenceDialog::on_closeButton_clicked);
}

void ReferenceDialog::setupColumns(const QStringList &columns, const QList<int> &widths)
{
    qDebug() << "ReferenceDialog::setupColumns" << "cols=" << columns.size();
    ui->tableWidget->setColumnCount(columns.size());
    ui->tableWidget->setHorizontalHeaderLabels(columns);

    QHeaderView *header = ui->tableWidget->horizontalHeader();
    const int n = columns.size();
    for (int i = 0; i < n; ++i) {
        if (i == 1) {
            header->setSectionResizeMode(i, QHeaderView::Stretch);
        } else {
            header->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    }

    // Скрываем колонку ID (индекс 0)
    if (columns.size() > 0) {
        qDebug() << "ReferenceDialog::setupColumns: branch hide_id_column";
        ui->tableWidget->setColumnHidden(0, true);
    }
}

int ReferenceDialog::currentRow() const
{
    return ui->tableWidget->currentRow();
}

QString ReferenceDialog::cellText(int row, int column) const
{
    QTableWidgetItem *item = ui->tableWidget->item(row, column);
    return item ? item->text() : QString();
}

void ReferenceDialog::setCellText(int row, int column, const QString &text)
{
    ui->tableWidget->setItem(row, column, new QTableWidgetItem(text));
}

int ReferenceDialog::insertRow()
{
    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    return row;
}

void ReferenceDialog::clearTable()
{
    ui->tableWidget->setRowCount(0);
    updateRecordCount();
}

void ReferenceDialog::showWarning(const QString &message)
{
    QMessageBox::warning(this, windowTitle(), message);
}

void ReferenceDialog::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Ошибка"), message);
}

void ReferenceDialog::showInfo(const QString &message)
{
    QMessageBox::information(this, windowTitle(), message);
}

bool ReferenceDialog::askConfirmation(const QString &message)
{
    return QMessageBox::question(this, windowTitle(), message,
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes;
}

void ReferenceDialog::updateRecordCount()
{
    qDebug() << "ReferenceDialog::updateRecordCount";
    const int total = ui->tableWidget->rowCount();
    int visible = 0;
    for (int i = 0; i < total; ++i) {
        if (!ui->tableWidget->isRowHidden(i))
            ++visible;
    }
    
    if (visible == total) {
        qDebug() << "ReferenceDialog::updateRecordCount: branch all_visible";
        ui->statusLabel->setText(tr("Записей: %1").arg(total));
    } else {
        qDebug() << "ReferenceDialog::updateRecordCount: branch filtered" << visible << total;
        ui->statusLabel->setText(tr("Записей: %1 из %2").arg(visible).arg(total));
    }
}

void ReferenceDialog::on_addButton_clicked()
{
    qDebug() << "ReferenceDialog::on_addButton_clicked";
    if (!m_core || !m_core->ensureSessionValid()) { qDebug() << "ReferenceDialog::on_addButton_clicked: branch session_invalid"; reject(); return; }
    addRecord();
}

void ReferenceDialog::on_editButton_clicked()
{
    qDebug() << "ReferenceDialog::on_editButton_clicked";
    if (!m_core || !m_core->ensureSessionValid()) { qDebug() << "ReferenceDialog::on_editButton_clicked: branch session_invalid"; reject(); return; }
    if (currentRow() < 0) {
        qDebug() << "ReferenceDialog::on_editButton_clicked: branch no_selection";
        showWarning(tr("Выберите запись для редактирования."));
        return;
    }
    editRecord();
}

void ReferenceDialog::on_deleteButton_clicked()
{
    qDebug() << "ReferenceDialog::on_deleteButton_clicked";
    if (!m_core || !m_core->ensureSessionValid()) { qDebug() << "ReferenceDialog::on_deleteButton_clicked: branch session_invalid"; reject(); return; }
    if (currentRow() < 0) {
        qDebug() << "ReferenceDialog::on_deleteButton_clicked: branch no_selection";
        showWarning(tr("Выберите запись для удаления."));
        return;
    }
    deleteRecord();
}

void ReferenceDialog::on_closeButton_clicked()
{
    qDebug() << "ReferenceDialog::on_closeButton_clicked";
    accept();
}

void ReferenceDialog::on_refreshButton_clicked()
{
    qDebug() << "ReferenceDialog::on_refreshButton_clicked";
    if (!m_core || !m_core->ensureSessionValid()) { qDebug() << "ReferenceDialog::on_refreshButton_clicked: branch session_invalid"; reject(); return; }
    loadTable(false);  // false = не показывать удалённые
}

void ReferenceDialog::on_searchButton_clicked()
{
    // Применяем текущий текст из поля поиска
    applyFilter(ui->searchEdit->text());
}

void ReferenceDialog::on_searchEdit_textChanged(const QString &text)
{
    m_lastSearchText = text;
    applyFilter(text);
}

void ReferenceDialog::on_searchEdit_returnPressed()
{
    // При нажатии Enter в поле поиска применяем фильтр
    applyFilter(ui->searchEdit->text());
}

void ReferenceDialog::on_tableWidget_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    qDebug() << "ReferenceDialog::on_tableWidget_doubleClicked";
    if (!m_core || !m_core->ensureSessionValid()) { qDebug() << "ReferenceDialog::on_tableWidget_doubleClicked: branch session_invalid"; reject(); return; }
    if (currentRow() >= 0) {
        qDebug() << "ReferenceDialog::on_tableWidget_doubleClicked: branch edit";
        editRecord();
    } else {
        qDebug() << "ReferenceDialog::on_tableWidget_doubleClicked: branch no_row";
    }
}

void ReferenceDialog::on_tableWidget_customContextMenuRequested(const QPoint &pos)
{
    qDebug() << "ReferenceDialog::on_tableWidget_customContextMenuRequested";
    QMenu menu(this);
    
    QAction *addAction = menu.addAction(tr("Добавить"));
    connect(addAction, &QAction::triggered, this, &ReferenceDialog::on_addButton_clicked);
    
    if (currentRow() >= 0) {
        qDebug() << "ReferenceDialog::on_tableWidget_customContextMenuRequested: branch has_selection";
        QAction *editAction = menu.addAction(tr("Изменить"));
        connect(editAction, &QAction::triggered, this, &ReferenceDialog::on_editButton_clicked);
        
        QAction *deleteAction = menu.addAction(tr("Удалить"));
        connect(deleteAction, &QAction::triggered, this, &ReferenceDialog::on_deleteButton_clicked);
        
        menu.addSeparator();
    } else {
        qDebug() << "ReferenceDialog::on_tableWidget_customContextMenuRequested: branch no_selection";
    }
    
    QAction *refreshAction = menu.addAction(tr("Обновить"));
    connect(refreshAction, &QAction::triggered, this, &ReferenceDialog::on_refreshButton_clicked);
    
    menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos));
}

void ReferenceDialog::applyFilter(const QString &searchText)
{
    qDebug() << "ReferenceDialog::applyFilter" << "text.length=" << searchText.length();
    const QString lowerSearch = searchText.toLower();
    
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        bool match = searchText.isEmpty();
        
        if (!match) {
            // Поиск по всем колонкам
            for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
                QTableWidgetItem *item = ui->tableWidget->item(row, col);
                if (item && item->text().toLower().contains(lowerSearch)) {
                    match = true;
                    break;
                }
            }
        }
        
        ui->tableWidget->setRowHidden(row, !match);
    }
    
    updateRecordCount();
}
