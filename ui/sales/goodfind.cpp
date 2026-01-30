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
    ui->tableWidget->setColumnHidden(4, false); // Кол-во видимо
    ui->tableWidget->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget->setColumnWidth(0, 70);
    ui->tableWidget->setColumnWidth(1, 220);
    ui->tableWidget->setColumnWidth(2, 140);
    ui->tableWidget->setColumnWidth(3, 80);
    ui->tableWidget->setColumnWidth(4, 60);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &GoodFind::on_searchEdit_textChanged);
    connect(ui->selectButton, &QPushButton::clicked, this, &GoodFind::on_selectButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &GoodFind::on_cancelButton_clicked);
    connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &GoodFind::on_tableWidget_itemDoubleClicked);

    fillTable();
}

GoodFind::~GoodFind()
{
    delete ui;
}

void GoodFind::fillTable()
{
    ui->tableWidget->setRowCount(0);
    if (!m_core) return;

    SalesManager *sm = m_core->createSalesManager(this);
    if (!sm) return;

    int row = 0;
    const QList<BatchInfo> batches = sm->getAvailableBatches();
    for (const BatchInfo &b : batches) {
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QStringLiteral("Товар")));
        QTableWidgetItem *nameItem = new QTableWidgetItem(b.goodName);
        nameItem->setIcon(iconFromImageUrl(b.imageUrl));
        ui->tableWidget->setItem(row, 1, nameItem);
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(b.categoryName));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(b.price, 'f', 2)));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(b.qnt)));
        QTableWidgetItem *typeItem = ui->tableWidget->item(row, 0);
        typeItem->setData(RoleType, QStringLiteral("batch"));
        typeItem->setData(RoleId, b.batchId);
        ui->tableWidget->setRowHeight(row, qMax(ui->tableWidget->rowHeight(row), kGoodFindIconSize + 4));
        row++;
    }

    const QList<ServiceInfo> services = sm->getAvailableServices();
    for (const ServiceInfo &s : services) {
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QStringLiteral("Услуга")));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(s.serviceName));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(s.categoryName));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(s.price, 'f', 2)));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QStringLiteral("—")));
        QTableWidgetItem *typeItem = ui->tableWidget->item(row, 0);
        typeItem->setData(RoleType, QStringLiteral("service"));
        typeItem->setData(RoleId, s.serviceId);
        row++;
    }

    applyFilter(ui->searchEdit->text().trimmed());
}

void GoodFind::applyFilter(const QString &filter)
{
    const QString f = filter.toLower().trimmed();
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        bool hide = false;
        if (!f.isEmpty()) {
            QTableWidgetItem *nameItem = ui->tableWidget->item(r, 1);
            QTableWidgetItem *catItem = ui->tableWidget->item(r, 2);
            QString name = nameItem ? nameItem->text().toLower() : QString();
            QString cat = catItem ? catItem->text().toLower() : QString();
            hide = !name.contains(f) && !cat.contains(f);
        }
        ui->tableWidget->setRowHidden(r, hide);
    }
}

void GoodFind::on_searchEdit_textChanged(const QString &text)
{
    applyFilter(text.trimmed());
}

void GoodFind::selectCurrentRow()
{
    const int row = ui->tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Поиск"), tr("Выберите строку в таблице."));
        return;
    }
    QTableWidgetItem *typeItem = ui->tableWidget->item(row, 0);
    if (!typeItem) return;
    const QString type = typeItem->data(RoleType).toString();
    const qint64 id = typeItem->data(RoleId).toLongLong();
    if (type == QLatin1String("batch")) {
        m_isBatch = true;
        m_batchId = id;
        m_serviceId = 0;
    } else {
        m_isBatch = false;
        m_batchId = 0;
        m_serviceId = id;
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
    selectCurrentRow();
}
