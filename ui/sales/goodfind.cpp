#include "goodfind.h"
#include "ui_goodfind.h"
#include "../../easyposcore.h"
#include "../../sales/salesmanager.h"
#include "../../sales/structures.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QFileInfo>
#include <QApplication>

// Роли для хранения типа и id в ячейке таблицы
enum { RoleType = Qt::UserRole, RoleId = Qt::UserRole + 1 };

namespace {
constexpr int kGoodFindIconSize = 28;

QIcon iconFromImageUrl(const QString &imageUrl)
{
    if (imageUrl.isEmpty())
        return QIcon();
    QString path = imageUrl;
    if (!QFileInfo(path).isAbsolute())
        path = QDir(QApplication::applicationDirPath()).absoluteFilePath(imageUrl);
    QPixmap pm(path);
    if (pm.isNull())
        return QIcon();
    return QIcon(pm.scaled(kGoodFindIconSize, kGoodFindIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
} // namespace

GoodFind::GoodFind(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::GoodFind)
    , m_core(core)
{
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget->setColumnWidth(0, 260);
    ui->tableWidget->setColumnWidth(1, 160);
    ui->tableWidget->setColumnWidth(2, 90);
    ui->tableWidget->setColumnWidth(3, 70);
    ui->tableWidgetServices->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidgetServices->setColumnWidth(0, 260);
    ui->tableWidgetServices->setColumnWidth(1, 160);
    ui->tableWidgetServices->setColumnWidth(2, 90);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &GoodFind::on_searchEdit_textChanged);
    connect(ui->selectButton, &QPushButton::clicked, this, &GoodFind::on_selectButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &GoodFind::on_cancelButton_clicked);
    connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &GoodFind::on_tableWidget_itemDoubleClicked);
    connect(ui->tableWidgetServices, &QTableWidget::itemDoubleClicked, this, &GoodFind::on_tableWidget_itemDoubleClicked);

    fillTable();
}

void GoodFind::setInitialSearch(const QString &text)
{
    ui->searchEdit->setText(text.trimmed());
    applyFilter(text.trimmed());
}

GoodFind::~GoodFind()
{
    delete ui;
}

void GoodFind::fillTable()
{
    ui->tableWidget->setRowCount(0);
    ui->tableWidgetServices->setRowCount(0);
    if (!m_core) return;

    SalesManager *sm = m_core->createSalesManager(this);
    if (!sm) return;

    const QList<BatchInfo> batches = sm->getAvailableBatches();
    for (const BatchInfo &b : batches) {
        const int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        QTableWidgetItem *nameItem = new QTableWidgetItem(b.goodName);
        nameItem->setIcon(iconFromImageUrl(b.imageUrl));
        nameItem->setData(RoleType, QStringLiteral("batch"));
        nameItem->setData(RoleId, b.batchId);
        ui->tableWidget->setItem(row, 0, nameItem);
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(b.categoryName));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(b.price, 'f', 2)));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(b.qnt)));
        ui->tableWidget->setRowHeight(row, qMax(ui->tableWidget->rowHeight(row), kGoodFindIconSize + 4));
    }

    const QList<ServiceInfo> services = sm->getAvailableServices();
    for (const ServiceInfo &s : services) {
        const int row = ui->tableWidgetServices->rowCount();
        ui->tableWidgetServices->insertRow(row);
        QTableWidgetItem *nameItem = new QTableWidgetItem(s.serviceName);
        nameItem->setData(RoleId, s.serviceId);
        ui->tableWidgetServices->setItem(row, 0, nameItem);
        ui->tableWidgetServices->setItem(row, 1, new QTableWidgetItem(s.categoryName));
        ui->tableWidgetServices->setItem(row, 2, new QTableWidgetItem(QString::number(s.price, 'f', 2)));
    }

    applyFilter(ui->searchEdit->text().trimmed());
}

void GoodFind::applyFilter(const QString &filter)
{
    const QString f = filter.toLower().trimmed();
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        bool hide = false;
        if (!f.isEmpty()) {
            QTableWidgetItem *nameItem = ui->tableWidget->item(r, 0);
            QTableWidgetItem *catItem = ui->tableWidget->item(r, 1);
            QString name = nameItem ? nameItem->text().toLower() : QString();
            QString cat = catItem ? catItem->text().toLower() : QString();
            hide = !name.contains(f) && !cat.contains(f);
        }
        ui->tableWidget->setRowHidden(r, hide);
    }
    for (int r = 0; r < ui->tableWidgetServices->rowCount(); ++r) {
        bool hide = false;
        if (!f.isEmpty()) {
            QTableWidgetItem *nameItem = ui->tableWidgetServices->item(r, 0);
            QTableWidgetItem *catItem = ui->tableWidgetServices->item(r, 1);
            QString name = nameItem ? nameItem->text().toLower() : QString();
            QString cat = catItem ? catItem->text().toLower() : QString();
            hide = !name.contains(f) && !cat.contains(f);
        }
        ui->tableWidgetServices->setRowHidden(r, hide);
    }
}

void GoodFind::on_searchEdit_textChanged(const QString &text)
{
    applyFilter(text.trimmed());
}

void GoodFind::selectCurrentRow()
{
    if (ui->tabWidget->currentIndex() == 0) {
        const int row = ui->tableWidget->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("Поиск"), tr("Выберите товар в таблице."));
            return;
        }
        QTableWidgetItem *nameItem = ui->tableWidget->item(row, 0);
        if (!nameItem) return;
        m_isBatch = true;
        m_batchId = nameItem->data(RoleId).toLongLong();
        m_serviceId = 0;
    } else {
        const int row = ui->tableWidgetServices->currentRow();
        if (row < 0) {
            QMessageBox::information(this, tr("Поиск"), tr("Выберите услугу в таблице."));
            return;
        }
        QTableWidgetItem *nameItem = ui->tableWidgetServices->item(row, 0);
        if (!nameItem) return;
        m_isBatch = false;
        m_batchId = 0;
        m_serviceId = nameItem->data(RoleId).toLongLong();
    }
    accept();
}

void GoodFind::on_selectButton_clicked()
{
    selectCurrentRow();
}

void GoodFind::on_cancelButton_clicked()
{
    m_isBatch = false;
    m_batchId = 0;
    m_serviceId = 0;
    reject();
}

void GoodFind::on_tableWidget_itemDoubleClicked(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    if (sender() == ui->tableWidgetServices)
        ui->tabWidget->setCurrentIndex(1);
    else
        ui->tabWidget->setCurrentIndex(0);
    selectCurrentRow();
}
