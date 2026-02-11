#include "reportsdialog.h"
#include "ui_reportsdialog.h"
#include "../easyposcore.h"
#include "../reports/reportmanager.h"
#include "../reports/reportdata.h"
#include "../reports/csvreportexporter.h"
#include "../reports/odtreportexporter.h"
#include "../shifts/shiftmanager.h"
#include "../shifts/structures.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTableWidgetItem>

enum ReportType {
    SalesByPeriod = 0,
    StockReceipt = 1,
    StockWriteOff = 2,
    SalesByShift = 3,
    Reconciliation = 4,
    StockBalance = 5
};

ReportsDialog::ReportsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::ReportsDialog)
    , m_core(core)
{
    ui->setupUi(this);
    QDate today = QDate::currentDate();
    ui->dateFromEdit->setDate(today.addDays(-6));
    ui->dateToEdit->setDate(today);

    connect(ui->reportTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportsDialog::onReportTypeChanged);
    connect(ui->generateButton, &QPushButton::clicked, this, &ReportsDialog::onGenerate);
    connect(ui->exportCsvButton, &QPushButton::clicked, this, &ReportsDialog::onExportCsv);
    connect(ui->exportOdtButton, &QPushButton::clicked, this, &ReportsDialog::onExportOdt);
    connect(ui->dateFromEdit, &QDateEdit::dateChanged, this, [this] { refreshShiftList(); });
    connect(ui->dateToEdit, &QDateEdit::dateChanged, this, [this] { refreshShiftList(); });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateParamsVisibility();
    refreshShiftList();
}

ReportsDialog::~ReportsDialog()
{
    delete ui;
}

void ReportsDialog::onReportTypeChanged(int index)
{
    Q_UNUSED(index);
    updateParamsVisibility();
}

void ReportsDialog::updateParamsVisibility()
{
    const int t = ui->reportTypeCombo->currentIndex();
    const bool needShift = (t == SalesByShift || t == Reconciliation);
    const bool needPeriod = (t != StockBalance); // для выгрузки остатков период не нужен
    ui->periodLabel->setVisible(needPeriod);
    ui->periodWidget->setVisible(needPeriod);
    ui->shiftLabel->setVisible(needShift);
    ui->shiftCombo->setVisible(needShift);
    if (needShift)
        refreshShiftList();
}

void ReportsDialog::refreshShiftList()
{
    ui->shiftCombo->clear();
    ui->shiftCombo->addItem(tr("— Выберите смену —"), 0);
    ShiftManager *sm = m_core ? m_core->getShiftManager() : nullptr;
    if (!sm) return;
    QDate from = ui->dateFromEdit->date();
    QDate to = ui->dateToEdit->date();
    if (from > to) to = from;
    QList<WorkShift> shifts = sm->getShifts(0, from, to);
    for (const WorkShift &s : shifts) {
        QString text = tr("ID %1 — %2, %3 %4")
            .arg(s.id)
            .arg(s.employeeName.isEmpty() ? QString::number(s.employeeId) : s.employeeName)
            .arg(s.startedDate.toString(tr("dd.MM.yyyy")))
            .arg(s.startedTime.toString(tr("HH:mm")));
        if (s.isActive())
            text += tr(" (открыта)");
        ui->shiftCombo->addItem(text, s.id);
    }
}

void ReportsDialog::fillPreviewFromReportData(const ReportData &data)
{
    ui->previewTable->setRowCount(0);
    ui->previewTable->setColumnCount(0);
    if (data.header.isEmpty()) return;
    ui->previewTable->setColumnCount(data.header.size());
    ui->previewTable->setHorizontalHeaderLabels(data.header);
    ui->previewTable->setRowCount(data.rows.size());
    for (int r = 0; r < data.rows.size(); ++r) {
        const QStringList &row = data.rows.at(r);
        for (int c = 0; c < row.size() && c < data.header.size(); ++c)
            ui->previewTable->setItem(r, c, new QTableWidgetItem(row.at(c)));
    }
    ui->previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->previewTable->horizontalHeader()->setStretchLastSection(true);
}

void ReportsDialog::onGenerate()
{
    if (!m_core) return;
    ReportManager *rm = m_core->createReportManager(this);
    if (!rm) {
        QMessageBox::warning(this, windowTitle(), tr("Не удалось создать отчёт."));
        return;
    }
    ReportData data;
    const int t = ui->reportTypeCombo->currentIndex();
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    switch (t) {
    case SalesByPeriod:
        data = rm->generateSalesByPeriodReport(from, to);
        break;
    case StockReceipt:
        data = rm->generateStockReceiptReport(from, to);
        break;
    case StockWriteOff:
        data = rm->generateStockWriteOffReport(from, to);
        break;
    case SalesByShift: {
        qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
        if (shiftId <= 0) {
            QMessageBox::information(this, windowTitle(), tr("Выберите смену."));
            return;
        }
        data = rm->generateSalesByShiftReport(shiftId);
        break;
    }
    case Reconciliation: {
        qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
        if (shiftId <= 0) {
            QMessageBox::information(this, windowTitle(), tr("Выберите смену."));
            return;
        }
        data = rm->generateReconciliationReport(shiftId);
        break;
    }
    case StockBalance:
        data = rm->generateStockBalanceReport();
        break;
    default:
        return;
    }
    fillPreviewFromReportData(data);
}

void ReportsDialog::onExportCsv()
{
    if (!m_core) return;
    ReportManager *rm = m_core->createReportManager(this);
    if (!rm) {
        QMessageBox::warning(this, windowTitle(), tr("Не удалось создать отчёт."));
        return;
    }
    ReportData data;
    const int t = ui->reportTypeCombo->currentIndex();
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    QString defaultName;
    switch (t) {
    case SalesByPeriod:
        data = rm->generateSalesByPeriodReport(from, to);
        defaultName = tr("Продажи_%1_%2.csv").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
        break;
    case StockReceipt:
        data = rm->generateStockReceiptReport(from, to);
        defaultName = tr("Приход_%1_%2.csv").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
        break;
    case StockWriteOff:
        data = rm->generateStockWriteOffReport(from, to);
        defaultName = tr("Списание_%1_%2.csv").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
        break;
    case SalesByShift: {
        qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
        if (shiftId <= 0) {
            QMessageBox::information(this, windowTitle(), tr("Выберите смену."));
            return;
        }
        data = rm->generateSalesByShiftReport(shiftId);
        defaultName = tr("Продажи_смена_%1.csv").arg(shiftId);
        break;
    }
    case Reconciliation: {
        qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
        if (shiftId <= 0) {
            QMessageBox::information(this, windowTitle(), tr("Выберите смену."));
            return;
        }
        data = rm->generateReconciliationReport(shiftId);
        defaultName = tr("Сверка_смена_%1.csv").arg(shiftId);
        break;
    }
    case StockBalance:
        data = rm->generateStockBalanceReport();
        defaultName = tr("Остатки_%1.csv").arg(QDate::currentDate().toString(Qt::ISODate));
        break;
    default:
        return;
    }
    if (data.rows.isEmpty() && data.header.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Экспорт в CSV"),
        defaultName, tr("CSV (*.csv)"), nullptr, QFileDialog::DontUseNativeDialog);
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

void ReportsDialog::onExportOdt()
{
    if (!m_core) return;
    ReportManager *rm = m_core->createReportManager(this);
    if (!rm) {
        QMessageBox::warning(this, windowTitle(), tr("Не удалось создать отчёт."));
        return;
    }
    ReportData data;
    const int t = ui->reportTypeCombo->currentIndex();
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    QString defaultName;
    switch (t) {
    case SalesByPeriod:
        data = rm->generateSalesByPeriodReport(from, to);
        defaultName = tr("Продажи_%1_%2.fodt").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
        break;
    case StockReceipt:
        data = rm->generateStockReceiptReport(from, to);
        defaultName = tr("Приход_%1_%2.fodt").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
        break;
    case StockWriteOff:
        data = rm->generateStockWriteOffReport(from, to);
        defaultName = tr("Списание_%1_%2.fodt").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
        break;
    case SalesByShift: {
        qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
        if (shiftId <= 0) {
            QMessageBox::information(this, windowTitle(), tr("Выберите смену."));
            return;
        }
        data = rm->generateSalesByShiftReport(shiftId);
        defaultName = tr("Продажи_смена_%1.fodt").arg(shiftId);
        break;
    }
    case Reconciliation: {
        qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
        if (shiftId <= 0) {
            QMessageBox::information(this, windowTitle(), tr("Выберите смену."));
            return;
        }
        data = rm->generateReconciliationReport(shiftId);
        defaultName = tr("Сверка_смена_%1.fodt").arg(shiftId);
        break;
    }
    case StockBalance:
        data = rm->generateStockBalanceReport();
        defaultName = tr("Остатки_%1.fodt").arg(QDate::currentDate().toString(Qt::ISODate));
        break;
    default:
        return;
    }
    data.preparedByName = m_core->getCurrentEmployeeDisplayName();
    data.preparedByPosition = m_core->getCurrentEmployeePositionName();
    if (data.rows.isEmpty() && data.header.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Сформировать отчёт в ODT"),
        defaultName, tr("Документ ODT (*.fodt *.odt);;Все файлы (*)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (path.isEmpty()) return;
    if (!path.endsWith(QLatin1String(".fodt"), Qt::CaseInsensitive) && !path.endsWith(QLatin1String(".odt"), Qt::CaseInsensitive))
        path += QLatin1String(".fodt");
    OdtReportExporter exporter;
    if (m_core) {
        QString logoPath = m_core->getBrandingLogoPath();
        if (!logoPath.isEmpty())
            exporter.setLogoPath(logoPath);
        QString appName = m_core->getBrandingAppName();
        if (!appName.isEmpty())
            exporter.setCompanyName(appName);
        QString addr = m_core->getBrandingAddress();
        if (!addr.isEmpty())
            exporter.setAddress(addr);
        QString legal = m_core->getBrandingLegalInfo();
        if (!legal.isEmpty())
            exporter.setLegalInfo(legal);
    }
    if (!exporter.exportReport(data, path)) {
        QMessageBox::critical(this, windowTitle(), tr("Не удалось создать файл."));
        return;
    }
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён: %1").arg(path));
}
