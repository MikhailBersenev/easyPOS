#include "goodfind.h"
#include "ui_goodfind.h"
#include "../../easyposcore.h"
#include "../../sales/salesmanager.h"
#include "../../sales/structures.h"
#include <QListWidgetItem>
#include <QMessageBox>
#include <QIcon>
#include <QPixmap>
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QStyle>

// Роли для хранения типа и id в элементе списка
enum { RoleType = Qt::UserRole, RoleId = Qt::UserRole + 1 };

namespace {
constexpr int kCardIconSize = 72;

// Набор стандартных пиктограмм для товаров/услуг без своего изображения (индекс выбирается по id — стабильно)
static const QStyle::StandardPixmap kRandomPictograms[] = {
    QStyle::SP_FileIcon,
    QStyle::SP_DirIcon,
    QStyle::SP_ComputerIcon,
    QStyle::SP_DesktopIcon,
    QStyle::SP_DriveFDIcon,
    QStyle::SP_DriveHDIcon,
    QStyle::SP_FileDialogContentsView,
    QStyle::SP_FileDialogListView,
    QStyle::SP_DirOpenIcon,
    QStyle::SP_FileLinkIcon,
};
static constexpr int kRandomPictogramsCount = sizeof(kRandomPictograms) / sizeof(kRandomPictograms[0]);

QIcon randomPictogramForId(qint64 id)
{
    const uint u = static_cast<uint>(id ^ (id >> 16));
    const int idx = static_cast<int>(u % static_cast<uint>(kRandomPictogramsCount));
    QIcon icon = QApplication::style()->standardIcon(kRandomPictograms[idx]);
    QPixmap pm = icon.pixmap(kCardIconSize, kCardIconSize);
    if (!pm.isNull())
        return QIcon(pm);
    return icon;
}

QIcon iconFromImageUrl(const QString &imageUrl, qint64 fallbackId)
{
    if (!imageUrl.isEmpty()) {
        QString path = imageUrl;
        if (!QFileInfo(path).isAbsolute())
            path = QDir(QApplication::applicationDirPath()).absoluteFilePath(imageUrl);
        QPixmap pm(path);
        if (!pm.isNull())
            return QIcon(pm.scaled(kCardIconSize, kCardIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    return randomPictogramForId(fallbackId);
}
} // namespace

GoodFind::GoodFind(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::GoodFind)
    , m_core(core)
{
    ui->setupUi(this);
    ui->listWidget->setWordWrap(true);
    ui->listWidgetServices->setWordWrap(true);

    connect(ui->searchEdit, &QLineEdit::textChanged, this, &GoodFind::on_searchEdit_textChanged);
    connect(ui->selectButton, &QPushButton::clicked, this, &GoodFind::on_selectButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &GoodFind::on_cancelButton_clicked);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &GoodFind::on_itemDoubleClicked);
    connect(ui->listWidgetServices, &QListWidget::itemDoubleClicked, this, &GoodFind::on_itemDoubleClicked);

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
    m_batches.clear();
    m_services.clear();
    if (!m_core) return;

    SalesManager *sm = m_core->createSalesManager(this);
    if (!sm) return;

    m_batches = sm->getAvailableBatches();
    m_services = sm->getAvailableServices();

    applyFilter(ui->searchEdit->text().trimmed());
}

void GoodFind::applyFilter(const QString &filter)
{
    const QString f = filter.toLower().trimmed();

    ui->listWidget->clear();
    for (const BatchInfo &b : m_batches) {
        if (!f.isEmpty()) {
            const bool match = b.goodName.toLower().contains(f) || b.categoryName.toLower().contains(f);
            if (!match) continue;
        }
        const QString title = b.goodName.length() > 20 ? b.goodName.left(19) + QLatin1String("…") : b.goodName;
        const QString priceStr = QString::number(b.price, 'f', 2) + QStringLiteral(" ₽");
        QListWidgetItem *item = new QListWidgetItem(iconFromImageUrl(b.imageUrl, b.batchId), title + QLatin1String("\n") + priceStr);
        item->setData(RoleType, QStringLiteral("batch"));
        item->setData(RoleId, b.batchId);
        item->setToolTip(tr("%1\nКатегория: %2\nЦена: %3 ₽\nВ наличии: %4")
                             .arg(b.goodName).arg(b.categoryName)
                             .arg(QString::number(b.price, 'f', 2)).arg(b.qnt));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        ui->listWidget->addItem(item);
    }

    ui->listWidgetServices->clear();
    for (const ServiceInfo &s : m_services) {
        if (!f.isEmpty()) {
            const bool match = s.serviceName.toLower().contains(f) || s.categoryName.toLower().contains(f);
            if (!match) continue;
        }
        const QString title = s.serviceName.length() > 20 ? s.serviceName.left(19) + QLatin1String("…") : s.serviceName;
        const QString priceStr = QString::number(s.price, 'f', 2) + QStringLiteral(" ₽");
        QListWidgetItem *item = new QListWidgetItem(randomPictogramForId(s.serviceId), title + QLatin1String("\n") + priceStr);
        item->setData(RoleType, QStringLiteral("service"));
        item->setData(RoleId, s.serviceId);
        item->setToolTip(tr("%1\nКатегория: %2\nЦена: %3 ₽")
                             .arg(s.serviceName).arg(s.categoryName)
                             .arg(QString::number(s.price, 'f', 2)));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        ui->listWidgetServices->addItem(item);
    }
}

void GoodFind::on_searchEdit_textChanged(const QString &text)
{
    applyFilter(text.trimmed());
}

void GoodFind::selectCurrentRow()
{
    if (ui->tabWidget->currentIndex() == 0) {
        QListWidgetItem *item = ui->listWidget->currentItem();
        if (!item) {
            QMessageBox::information(this, tr("Поиск"), tr("Выберите товар."));
            return;
        }
        m_isBatch = true;
        m_batchId = item->data(RoleId).toLongLong();
        m_serviceId = 0;
    } else {
        QListWidgetItem *item = ui->listWidgetServices->currentItem();
        if (!item) {
            QMessageBox::information(this, tr("Поиск"), tr("Выберите услугу."));
            return;
        }
        m_isBatch = false;
        m_batchId = 0;
        m_serviceId = item->data(RoleId).toLongLong();
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

void GoodFind::on_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    if (sender() == ui->listWidgetServices)
        ui->tabWidget->setCurrentIndex(1);
    else
        ui->tabWidget->setCurrentIndex(0);
    selectCurrentRow();
}
