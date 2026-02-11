#include "salesreportdialog.h"
#include "ui_salesreportdialog.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include "../reports/reportmanager.h"
#include "../reports/csvreportexporter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QLocale>

SalesReportDialog::SalesReportDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::SalesReportDialog)
    , m_core(core)
{
    ui->setupUi(this);
    ui->dateFromEdit->setDate(QDate::currentDate().addDays(-6));
    ui->dateToEdit->setDate(QDate::currentDate());
    connect(ui->generateButton, &QPushButton::clicked, this, &SalesReportDialog::onGenerate);
    connect(ui->exportButton, &QPushButton::clicked, this, &SalesReportDialog::onExportCsv);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    onGenerate();
}

SalesReportDialog::~SalesReportDialog()
{
    delete ui;
}

void SalesReportDialog::onGenerate()
{
    ui->reportEdit->clear();
    m_lastChecks.clear();

    if (!m_core || !m_core->getDatabaseConnection()) return;
    auto *sm = m_core->createSalesManager(this);
    if (!sm) return;

    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    if (from > to) {
        ui->reportEdit->setPlainText(tr("Дата начала не может быть позже даты окончания."));
        return;
    }

    m_lastChecks = sm->getChecks(from, to, -1, false);

    double totalAmount = 0;
    double totalDiscount = 0;
    double totalGoods = 0;
    double totalServices = 0;

    for (const Check &ch : m_lastChecks) {
        totalAmount += ch.totalAmount;
        totalDiscount += ch.discountAmount;
        const QList<SaleRow> rows = sm->getSalesByCheck(ch.id, false);
        for (const SaleRow &row : rows) {
            if (row.serviceId != 0)
                totalServices += row.sum;
            else
                totalGoods += row.sum;
        }
    }
    const double toPay = totalAmount - totalDiscount;

    QString report;
    report += tr("Отчёт по продажам\n");
    report += tr("Период: %1 — %2\n\n")
        .arg(QLocale::system().toString(from, QLocale::ShortFormat))
        .arg(QLocale::system().toString(to, QLocale::ShortFormat));
    report += tr("Чеков: %1\n").arg(m_lastChecks.size());
    report += tr("Сумма продаж: %1 ₽\n").arg(QString::number(totalAmount, 'f', 2));
    report += tr("  в т.ч. товары: %1 ₽\n").arg(QString::number(totalGoods, 'f', 2));
    report += tr("  в т.ч. услуги: %1 ₽\n").arg(QString::number(totalServices, 'f', 2));
    report += tr("Скидки: %1 ₽\n").arg(QString::number(totalDiscount, 'f', 2));
    report += tr("К оплате (итого): %1 ₽\n").arg(QString::number(toPay, 'f', 2));

    ui->reportEdit->setPlainText(report);
}

void SalesReportDialog::onExportCsv()
{
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    ReportManager *rm = m_core ? m_core->createReportManager(this) : nullptr;
    if (!rm) {
        QMessageBox::warning(this, windowTitle(), tr("Не удалось создать отчёт."));
        return;
    }
    ReportData data = rm->generateSalesByPeriodReport(from, to);
    if (data.rows.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    QString defaultName = tr("Продажи_%1_%2.csv")
        .arg(from.toString(Qt::ISODate))
        .arg(to.toString(Qt::ISODate));
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
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён."));
}
