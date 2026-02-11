#include "reportwizarddialog.h"
#include "../easyposcore.h"
#include "../reports/reportmanager.h"
#include "../reports/reportdata.h"
#include "../reports/csvreportexporter.h"
#include "../reports/odtreportexporter.h"
#include "../shifts/shiftmanager.h"
#include "../shifts/structures.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDateEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDate>
#include <QFont>
#include <QFontDatabase>
#include <QEvent>

enum ReportType {
    SalesByPeriod = 0,
    StockReceipt = 1,
    StockWriteOff = 2,
    SalesByShift = 3,
    Reconciliation = 4,
    StockBalance = 5,
    StockBalanceWithBarcodes = 6,
    Checks = 7,
    Production = 8
};

// --- TypePage ---
TypePage::TypePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Выбор типа отчёта"));
    setSubTitle(tr("Выберите отчёт, который хотите сформировать."));

    m_list = new QListWidget(this);
    // Устанавливаем шрифт для списка с поддержкой кириллицы
    QFont listFont = m_list->font();
    listFont.setPointSize(13);
    // Пробуем найти подходящий шрифт с поддержкой кириллицы
    const QStringList fontFamilies = { QStringLiteral("Roboto"), QStringLiteral("Segoe UI"), 
                                       QStringLiteral("Ubuntu"), QStringLiteral("Liberation Sans"),
                                       QStringLiteral("DejaVu Sans"), QStringLiteral("Arial") };
    QFontDatabase fontDb;
    for (const QString &family : fontFamilies) {
        if (fontDb.hasFamily(family)) {
            listFont.setFamily(family);
            break;
        }
    }
    m_list->setFont(listFont);
    
    m_list->addItem(tr("Продажи за период"));
    m_list->addItem(tr("Приход товаров на остатки"));
    m_list->addItem(tr("Списание с остатков"));
    m_list->addItem(tr("Продажи за смену"));
    m_list->addItem(tr("Сверка итогов (смена)"));
    m_list->addItem(tr("Выгрузка остатков"));
    m_list->addItem(tr("Выгрузка остатков по партиям (со штрихкодами)"));
    m_list->addItem(tr("Отчёт о чеках за смену или период"));
    m_list->addItem(tr("Отчёт о произведённой продукции"));
    m_list->setCurrentRow(0);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_list);
    registerField(QStringLiteral("reportType"), this, "reportType");
}

int TypePage::reportType() const
{
    return m_list->currentRow();
}

void TypePage::setReportType(int type)
{
    if (type >= 0 && type < m_list->count())
        m_list->setCurrentRow(type);
}

// --- ParamsPage ---
ParamsPage::ParamsPage(ReportWizardDialog *wizard, QWidget *parent)
    : QWizardPage(parent)
    , m_wizard(wizard)
{
    setTitle(tr("Параметры отчёта"));
    setSubTitle(tr("Укажите период или смену."));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_periodWidget = new QWidget(this);
    QFormLayout *periodForm = new QFormLayout(m_periodWidget);
    QHBoxLayout *periodRow = new QHBoxLayout();
    m_dateFrom = new QDateEdit(this);
    m_dateFrom->setCalendarPopup(true);
    QDate today = QDate::currentDate();
    m_dateFrom->setDate(today.addDays(-6));
    m_dateTo = new QDateEdit(this);
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDate(today);
    periodRow->addWidget(m_dateFrom);
    periodRow->addWidget(new QLabel(tr("—")));
    periodRow->addWidget(m_dateTo);
    periodForm->addRow(tr("Период:"), periodRow);
    mainLayout->addWidget(m_periodWidget);

    m_shiftWidget = new QWidget(this);
    QFormLayout *shiftForm = new QFormLayout(m_shiftWidget);
    m_shiftCombo = new QComboBox(this);
    m_shiftCombo->setMinimumWidth(280);
    shiftForm->addRow(tr("Смена:"), m_shiftCombo);
    mainLayout->addWidget(m_shiftWidget);

    mainLayout->addStretch();
}

void ParamsPage::updateForReportType(int type)
{
    const bool needShift = (type == SalesByShift || type == Reconciliation || type == Checks);
    const bool needPeriod = (type != StockBalance && type != StockBalanceWithBarcodes);
    
    m_periodWidget->setVisible(needPeriod);
    m_shiftWidget->setVisible(needShift);
    
    // Обновляем подзаголовок в зависимости от того, что нужно
    if (!needPeriod && !needShift) {
        // Для остатков параметры не нужны
        setSubTitle(tr("Параметры не требуются."));
    } else if (needShift && needPeriod) {
        // Нужны и период, и смена (для Checks)
        setSubTitle(tr("Укажите период и/или смену."));
    } else if (needShift) {
        // Нужна только смена
        setSubTitle(tr("Укажите смену."));
    } else {
        // Нужен только период
        setSubTitle(tr("Укажите период."));
    }
}

QDate ParamsPage::dateFrom() const { return m_dateFrom->date(); }
QDate ParamsPage::dateTo() const { return m_dateTo->date(); }

void ParamsPage::setDates(const QDate &from, const QDate &to)
{
    m_dateFrom->setDate(from);
    m_dateTo->setDate(to);
}

void ParamsPage::setShiftId(qint64 shiftId)
{
    int idx = m_shiftCombo->findData(shiftId);
    if (idx >= 0)
        m_shiftCombo->setCurrentIndex(idx);
}

qint64 ParamsPage::shiftId() const
{
    return m_shiftCombo->currentData().toLongLong();
}

bool ParamsPage::validatePage()
{
    if (!m_wizard) return true;
    TypePage *tp = qobject_cast<TypePage *>(m_wizard->page(0));
    if (!tp) return true;
    const int t = tp->reportType();
    // Для Checks смена необязательна (если не выбрана — отчёт за период)
    if (t == SalesByShift || t == Reconciliation) {
        if (m_shiftCombo->currentData().toLongLong() <= 0) {
            QMessageBox::warning(this, tr("Параметры"), tr("Выберите смену."));
            return false;
        }
    }
    return true;
}

void ParamsPage::refreshShiftList(EasyPOSCore *core)
{
    m_shiftCombo->clear();
    m_shiftCombo->addItem(tr("— Выберите смену —"), 0);
    if (!core) return;
    ShiftManager *sm = core->getShiftManager();
    if (!sm) return;
    QDate from = m_dateFrom->date();
    QDate to = m_dateTo->date();
    if (from > to) to = from;
    for (const WorkShift &s : sm->getShifts(0, from, to)) {
        QString text = tr("ID %1 — %2, %3 %4")
            .arg(s.id).arg(s.employeeName.isEmpty() ? QString::number(s.employeeId) : s.employeeName)
            .arg(s.startedDate.toString(tr("dd.MM.yyyy"))).arg(s.startedTime.toString(tr("HH:mm")));
        if (s.isActive()) text += tr(" (открыта)");
        m_shiftCombo->addItem(text, s.id);
    }
}

// --- ExportPage ---
ExportPage::ExportPage(ReportWizardDialog *wizard, QWidget *parent)
    : QWizardPage(parent)
    , m_wizard(wizard)
{
    setTitle(tr("Превью и экспорт"));
    setSubTitle(tr("Проверьте данные и экспортируйте отчёт в нужный формат."));

    QVBoxLayout *layout = new QVBoxLayout(this);
    m_previewTable = new QTableWidget(this);
    m_previewTable->setAlternatingRowColors(true);
    layout->addWidget(m_previewTable);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *csvBtn = new QPushButton(tr("Экспорт в CSV"), this);
    QPushButton *odtBtn = new QPushButton(tr("Отчёт в ODT"), this);
    connect(csvBtn, &QPushButton::clicked, this, &ExportPage::onExportCsv);
    connect(odtBtn, &QPushButton::clicked, this, &ExportPage::onExportOdt);
    buttonsLayout->addWidget(csvBtn);
    buttonsLayout->addWidget(odtBtn);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);
}

void ExportPage::fillFromReportData(const ReportData &data)
{
    m_previewTable->setRowCount(0);
    m_previewTable->setColumnCount(0);
    if (data.header.isEmpty()) return;
    m_previewTable->setColumnCount(data.header.size());
    m_previewTable->setHorizontalHeaderLabels(data.header);
    m_previewTable->setRowCount(data.rows.size());
    for (int r = 0; r < data.rows.size(); ++r) {
        const QStringList &row = data.rows.at(r);
        for (int c = 0; c < row.size() && c < data.header.size(); ++c)
            m_previewTable->setItem(r, c, new QTableWidgetItem(row.at(c)));
    }
    m_previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
}

void ExportPage::onExportCsv()
{
    ReportData data = m_wizard->generateReport();
    if (data.rows.isEmpty() && data.header.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    QString defaultName = data.suggestedFilename.isEmpty() ? tr("Otchet.csv") : data.suggestedFilename + QLatin1String(".csv");
    QString path = QFileDialog::getSaveFileName(this, tr("Экспорт в CSV"),
        defaultName, tr("CSV (*.csv)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
        path += QLatin1String(".csv");
    CsvReportExporter exporter;
    if (!exporter.exportReport(data, path)) {
        QMessageBox::critical(this, windowTitle(), tr("Не удалось создать файл."));
        return;
    }
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён: %1").arg(path));
}

void ExportPage::onExportOdt()
{
    ReportData data = m_wizard->generateReport();
    if (data.rows.isEmpty() && data.header.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    QString defaultName = data.suggestedFilename.isEmpty() ? tr("Otchet.fodt") : data.suggestedFilename + QLatin1String(".fodt");
    QString path = QFileDialog::getSaveFileName(this, tr("Отчёт в ODT"), defaultName,
        tr("Документ ODT (*.fodt *.odt);;Все файлы (*)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".fodt"), Qt::CaseInsensitive) && !path.endsWith(QLatin1String(".odt"), Qt::CaseInsensitive))
        path += QLatin1String(".fodt");
    EasyPOSCore *core = m_wizard->core();
    if (core) {
        data.preparedByName = core->getCurrentEmployeeDisplayName();
        data.preparedByPosition = core->getCurrentEmployeePositionName();
    }
    OdtReportExporter exporter;
    if (core) {
        if (!core->getBrandingLogoPath().isEmpty()) exporter.setLogoPath(core->getBrandingLogoPath());
        if (!core->getBrandingAppName().isEmpty()) exporter.setCompanyName(core->getBrandingAppName());
        if (!core->getBrandingAddress().isEmpty()) exporter.setAddress(core->getBrandingAddress());
        if (!core->getBrandingLegalInfo().isEmpty()) exporter.setLegalInfo(core->getBrandingLegalInfo());
    }
    if (!exporter.exportReport(data, path)) {
        QMessageBox::critical(this, windowTitle(), tr("Не удалось создать файл."));
        return;
    }
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён: %1").arg(path));
}

// Нужен публичный доступ к core для ExportPage
EasyPOSCore *ReportWizardDialog::core() const
{
    return m_core.get();
}

// --- ReportWizardDialog ---
ReportWizardDialog::ReportWizardDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QWizard(parent)
    , m_core(core)
{
    setWindowTitle(tr("Мастер отчётов"));
    setMinimumSize(580, 420);
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::IndependentPages, false);

    setupPages();
    connect(this, &QWizard::currentIdChanged, this, &ReportWizardDialog::onCurrentIdChanged);
}

void ReportWizardDialog::setPreset(ReportWizardType type, const QDate &dateFrom, const QDate &dateTo, qint64 shiftId, int startPage)
{
    m_typePage->setReportType(static_cast<int>(type));
    m_paramsPage->updateForReportType(static_cast<int>(type));
    m_paramsPage->setDates(dateFrom, dateTo);
    if (type == ReportSalesByShift || type == ReportReconciliation)
        m_paramsPage->refreshShiftList(m_core.get());
    m_paramsPage->setShiftId(shiftId);
    if (startPage >= 0 && startPage <= 2) {
        setStartId(startPage);
        if (startPage == 2)
            m_exportPage->fillFromReportData(generateReport());
    }
}

ReportWizardDialog::~ReportWizardDialog()
{
}

void ReportWizardDialog::setupPages()
{
    m_typePage = new TypePage(this);
    addPage(m_typePage);

    m_paramsPage = new ParamsPage(this, this);
    addPage(m_paramsPage);

    m_exportPage = new ExportPage(this, this);
    addPage(m_exportPage);
}

void ReportWizardDialog::onCurrentIdChanged(int id)
{
    if (page(id) == m_paramsPage) {
        const int t = m_typePage->reportType();
        m_paramsPage->updateForReportType(t);
        if (t == SalesByShift || t == Reconciliation || t == Checks)
            m_paramsPage->refreshShiftList(m_core.get());
    } else if (page(id) == m_exportPage) {
        m_exportPage->fillFromReportData(generateReport());
    }
}

ReportData ReportWizardDialog::generateReport() const
{
    ReportData empty;
    if (!m_core) return empty;
    ReportManager *rm = m_core->createReportManager(const_cast<ReportWizardDialog *>(this));
    if (!rm) return empty;

    const int t = m_typePage->reportType();
    const QDate from = m_paramsPage->dateFrom();
    const QDate to = m_paramsPage->dateTo();
    const qint64 shiftId = m_paramsPage->shiftId();

    switch (t) {
    case SalesByPeriod:
        return rm->generateSalesByPeriodReport(from, to);
    case StockReceipt:
        return rm->generateStockReceiptReport(from, to);
    case StockWriteOff:
        return rm->generateStockWriteOffReport(from, to);
    case SalesByShift:
        if (shiftId > 0) return rm->generateSalesByShiftReport(shiftId);
        return empty;
    case Reconciliation:
        if (shiftId > 0) return rm->generateReconciliationReport(shiftId);
        return empty;
    case StockBalance:
        return rm->generateStockBalanceReport();
    case StockBalanceWithBarcodes:
        return rm->generateStockBalanceWithBarcodesReport();
    case Checks:
        return rm->generateChecksReport(from, to, shiftId > 0 ? shiftId : -1);
    case Production:
        return rm->generateProductionReport(from, to);
    default:
        return empty;
    }
}

void ReportWizardDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        // Обновляем страницы мастера - пересоздаём их для обновления текстов
        setupPages();
        setWindowTitle(tr("Мастер отчётов"));
    }
    QWizard::changeEvent(event);
}

