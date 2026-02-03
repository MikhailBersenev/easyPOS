#include "salesreportdialog.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QDateEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QLocale>
#include <QTextStream>
#include <QStringConverter>

class SalesReportDialogPrivate
{
public:
    std::shared_ptr<EasyPOSCore> core;
    QDateEdit *dateFromEdit;
    QDateEdit *dateToEdit;
    QTextEdit *reportEdit;
    QList<Check> lastChecks;
};

SalesReportDialog::SalesReportDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , d(new SalesReportDialogPrivate)
{
    d->core = core;
    setWindowTitle(tr("Отчёт по продажам"));
    setMinimumSize(480, 400);

    d->dateFromEdit = new QDateEdit(this);
    d->dateFromEdit->setCalendarPopup(true);
    d->dateFromEdit->setDate(QDate::currentDate().addDays(-6));

    d->dateToEdit = new QDateEdit(this);
    d->dateToEdit->setCalendarPopup(true);
    d->dateToEdit->setDate(QDate::currentDate());

    d->reportEdit = new QTextEdit(this);
    d->reportEdit->setReadOnly(true);

    auto *btnGenerate = new QPushButton(tr("Сформировать"));
    connect(btnGenerate, &QPushButton::clicked, this, &SalesReportDialog::onGenerate);

    auto *btnExport = new QPushButton(tr("Экспорт в CSV"));
    connect(btnExport, &QPushButton::clicked, this, &SalesReportDialog::onExportCsv);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel(tr("Период:")));
    topLayout->addWidget(d->dateFromEdit);
    topLayout->addWidget(new QLabel(tr("—")));
    topLayout->addWidget(d->dateToEdit);
    topLayout->addWidget(btnGenerate);
    topLayout->addStretch();
    topLayout->addWidget(btnExport);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topLayout);
    layout->addWidget(d->reportEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    onGenerate();
}

SalesReportDialog::~SalesReportDialog()
{
    delete d;
}

void SalesReportDialog::onGenerate()
{
    d->reportEdit->clear();
    d->lastChecks.clear();

    if (!d->core || !d->core->getDatabaseConnection()) return;
    auto *sm = d->core->createSalesManager(this);
    if (!sm) return;

    const QDate from = d->dateFromEdit->date();
    const QDate to = d->dateToEdit->date();
    if (from > to) {
        d->reportEdit->setPlainText(tr("Дата начала не может быть позже даты окончания."));
        return;
    }

    d->lastChecks = sm->getChecks(from, to, -1, false);

    double totalAmount = 0;
    double totalDiscount = 0;
    for (const Check &ch : d->lastChecks) {
        totalAmount += ch.totalAmount;
        totalDiscount += ch.discountAmount;
    }
    const double toPay = totalAmount - totalDiscount;

    QString report;
    report += tr("Отчёт по продажам\n");
    report += tr("Период: %1 — %2\n\n")
        .arg(QLocale::system().toString(from, QLocale::ShortFormat))
        .arg(QLocale::system().toString(to, QLocale::ShortFormat));
    report += tr("Чеков: %1\n").arg(d->lastChecks.size());
    report += tr("Сумма продаж: %1 ₽\n").arg(QString::number(totalAmount, 'f', 2));
    report += tr("Скидки: %1 ₽\n").arg(QString::number(totalDiscount, 'f', 2));
    report += tr("К оплате (итого): %1 ₽\n").arg(QString::number(toPay, 'f', 2));

    d->reportEdit->setPlainText(report);
}

void SalesReportDialog::onExportCsv()
{
    if (d->lastChecks.isEmpty()) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    const QDate from = d->dateFromEdit->date();
    const QDate to = d->dateToEdit->date();
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
    for (const Check &ch : d->lastChecks) {
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
