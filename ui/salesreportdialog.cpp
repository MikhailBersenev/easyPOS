#include "salesreportdialog.h"
#include "ui_salesreportdialog.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QLocale>
#include <QTextStream>
#include <QStringConverter>

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
    if (m_lastChecks.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    QString defaultName = tr("Продажи_%1_%2.csv")
        .arg(from.toString(Qt::ISODate))
        .arg(to.toString(Qt::ISODate));
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
    out << "№ чека;Дата;Время;Сотрудник;Сумма;Скидка;К оплате\n";
    for (const Check &ch : m_lastChecks) {
        const double pay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);
        out << ch.id << ";"
            << ch.date.toString(Qt::ISODate) << ";"
            << ch.time.toString(Qt::ISODate) << ";"
            << "\"" << QString(ch.employeeName).replace(QLatin1String("\""), QLatin1String("\"\"")) << "\";"
            << QString::number(ch.totalAmount, 'f', 2) << ";"
            << QString::number(ch.discountAmount, 'f', 2) << ";"
            << QString::number(pay, 'f', 2) << "\n";
    }
    f.close();
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён."));
}
