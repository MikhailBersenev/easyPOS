#include "alertsdialog.h"
#include "ui_alertsdialog.h"
#include "../easyposcore.h"
#include "../alerts/alertsmanager.h"
#include "../alerts/alertkeys.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QStringConverter>
#include <algorithm>

static const int MAX_LOG_PREVIEW = 80;

AlertsDialog::AlertsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::AlertsDialog)
    , m_core(core)
{
    ui->setupUi(this);
    QDate today = QDate::currentDate();
    ui->dateFromEdit->setDate(today.addDays(-7));
    ui->dateToEdit->setDate(today);

    ui->table->horizontalHeader()->setStretchLastSection(true);
    ui->table->setAlternatingRowColors(true);
    ui->table->setColumnWidth(0, 50);
    ui->table->setColumnWidth(1, 90);
    ui->table->setColumnWidth(2, 70);
    ui->table->setColumnWidth(3, 90);
    ui->table->setColumnWidth(4, 140);
    ui->table->setColumnWidth(5, 100);
    ui->table->setColumnWidth(6, 120);
    ui->table->setSortingEnabled(true);
    ui->table->setColumnHidden(0, true); // ID скрыт

    fillCategoryCombo();

    connect(ui->dateFromEdit, &QDateEdit::dateChanged, this, &AlertsDialog::onFilterChanged);
    connect(ui->dateToEdit, &QDateEdit::dateChanged, this, &AlertsDialog::onFilterChanged);
    connect(ui->categoryCombo, &QComboBox::currentIndexChanged, this, &AlertsDialog::onFilterChanged);
    connect(ui->refreshButton, &QPushButton::clicked, this, &AlertsDialog::refreshTable);
    connect(ui->exportButton, &QPushButton::clicked, this, &AlertsDialog::onExportCsv);
    connect(ui->table, &QTableWidget::doubleClicked, this, &AlertsDialog::onRowDoubleClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refreshTable();
}

AlertsDialog::~AlertsDialog()
{
    delete ui;
}

void AlertsDialog::fillCategoryCombo()
{
    ui->categoryCombo->clear();
    ui->categoryCombo->addItem(tr("Все категории"), QString());
    ui->categoryCombo->addItem(tr("Аутентификация"), QString::fromUtf8(AlertCategory::Auth));
    ui->categoryCombo->addItem(tr("Продажи"), QString::fromUtf8(AlertCategory::Sales));
    ui->categoryCombo->addItem(tr("Смены"), QString::fromUtf8(AlertCategory::Shift));
    ui->categoryCombo->addItem(tr("Производство"), QString::fromUtf8(AlertCategory::Production));
    ui->categoryCombo->addItem(tr("Склад"), QString::fromUtf8(AlertCategory::Stock));
    ui->categoryCombo->addItem(tr("Справочники"), QString::fromUtf8(AlertCategory::Reference));
    ui->categoryCombo->addItem(tr("Система"), QString::fromUtf8(AlertCategory::System));
}

QString AlertsDialog::categoryFilterValue() const
{
    return ui->categoryCombo->currentData().toString();
}

void AlertsDialog::onFilterChanged()
{
    refreshTable();
}

void AlertsDialog::refreshTable()
{
    ui->statusLabel->clear();
    if (!m_core) {
        ui->table->setRowCount(0);
        return;
    }
    AlertsManager *am = m_core->createAlertsManager();
    if (!am) {
        ui->table->setRowCount(0);
        return;
    }

    const QDate dateFrom = ui->dateFromEdit->date();
    const QDate dateTo = ui->dateToEdit->date();
    const QString category = categoryFilterValue();
    const int limit = 2000;

    m_lastAlerts = am->getAlerts(dateFrom, dateTo, category, 0, 0, limit);

    ui->table->setSortingEnabled(false);
    ui->table->setRowCount(0);
    for (const Alert &a : m_lastAlerts) {
        const int row = ui->table->rowCount();
        ui->table->insertRow(row);
        ui->table->setItem(row, 0, new QTableWidgetItem(QString::number(a.id)));
        ui->table->setItem(row, 1, new QTableWidgetItem(a.eventDate.toString(Qt::ISODate)));
        ui->table->setItem(row, 2, new QTableWidgetItem(a.eventTime.toString(QStringLiteral("HH:mm:ss"))));
        ui->table->setItem(row, 3, new QTableWidgetItem(a.category));
        ui->table->setItem(row, 4, new QTableWidgetItem(a.signature));
        ui->table->setItem(row, 5, new QTableWidgetItem(a.userName));
        ui->table->setItem(row, 6, new QTableWidgetItem(a.employeeName));
        QString preview = a.fullLog;
        if (preview.length() > MAX_LOG_PREVIEW)
            preview = preview.left(MAX_LOG_PREVIEW) + QStringLiteral("…");
        ui->table->setItem(row, 7, new QTableWidgetItem(preview));
    }
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(1, Qt::DescendingOrder);
    if (ui->table->columnCount() > 2)
        ui->table->sortByColumn(2, Qt::DescendingOrder);

    ui->statusLabel->setText(tr("Событий: %1").arg(m_lastAlerts.size()));
    if (m_lastAlerts.size() >= limit)
        ui->statusLabel->setText(ui->statusLabel->text() + tr(" (показаны последние %1)").arg(limit));
}

void AlertsDialog::onRowDoubleClicked()
{
    const int row = ui->table->currentRow();
    if (row < 0) return;
    QTableWidgetItem *idItem = ui->table->item(row, 0);
    if (!idItem) return;
    const qint64 alertId = idItem->text().toLongLong();
    if (alertId <= 0) return;
    const auto it = std::find_if(m_lastAlerts.cbegin(), m_lastAlerts.cend(),
                                  [alertId](const Alert &a) { return a.id == alertId; });
    if (it == m_lastAlerts.cend()) return;
    const Alert &a = *it;
    QString text = tr("Дата: %1 %2\nКатегория: %3\nСобытие: %4\nПользователь: %5\nСотрудник: %6\n\nПолный лог:\n%7")
        .arg(a.eventDate.toString(Qt::ISODate),
             a.eventTime.toString(QStringLiteral("HH:mm:ss")),
             a.category,
             a.signature,
             a.userName.isEmpty() ? tr("—") : a.userName,
             a.employeeName.isEmpty() ? tr("—") : a.employeeName,
             a.fullLog.isEmpty() ? tr("—") : a.fullLog);
    QMessageBox::information(this, tr("Событие #%1").arg(a.id), text);
}

void AlertsDialog::onExportCsv()
{
    if (ui->table->rowCount() == 0) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    QString defaultName = tr("События_%1_%2.csv").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
    QString path = QFileDialog::getSaveFileName(this, tr("Экспорт в CSV"),
        defaultName, tr("CSV (*.csv)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
        path += QLatin1String(".csv");

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, windowTitle(), tr("Не удалось создать файл."));
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << tr("ID;Дата;Время;Категория;Событие;Пользователь;Сотрудник;Сообщение\n");
    for (int r = 0; r < ui->table->rowCount(); ++r) {
        auto cell = [this, r](int col) -> QString {
            QTableWidgetItem *item = ui->table->item(r, col);
            QString s = item ? item->text() : QString();
            return QStringLiteral("\"%1\"").arg(s.replace(QLatin1String("\""), QLatin1String("\"\"")));
        };
        out << cell(0) << ";" << cell(1) << ";" << cell(2) << ";" << cell(3) << ";"
            << cell(4) << ";" << cell(5) << ";" << cell(6) << ";" << cell(7) << "\n";
    }
    f.close();
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён."));
}
