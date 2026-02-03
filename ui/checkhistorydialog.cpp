#include "checkhistorydialog.h"
#include "checkdetailsdialog.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QDateEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QLocale>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QStringConverter>

class CheckHistoryDialogPrivate
{
public:
    std::shared_ptr<EasyPOSCore> core;
    QDateEdit *dateEdit;
    QTableWidget *table;
    QLabel *totalLabel;
};

CheckHistoryDialog::CheckHistoryDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , d(new CheckHistoryDialogPrivate)
{
    d->core = core;
    setWindowTitle(tr("История чеков"));
    setMinimumSize(500, 400);

    d->dateEdit = new QDateEdit(this);
    d->dateEdit->setCalendarPopup(true);
    d->dateEdit->setDate(QDate::currentDate());
    connect(d->dateEdit, &QDateEdit::dateChanged, this, &CheckHistoryDialog::onDateChanged);

    d->table = new QTableWidget(this);
    d->table->setColumnCount(5);
    d->table->setHorizontalHeaderLabels(
        { tr("№"), tr("Время"), tr("Сумма"), tr("Скидка"), tr("Сотрудник") });
    d->table->horizontalHeader()->setStretchLastSection(true);
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setSelectionMode(QAbstractItemView::SingleSelection);
    d->table->setAlternatingRowColors(true);
    connect(d->table, &QTableWidget::doubleClicked, this, [this](const QModelIndex &) {
        const int row = d->table->currentRow();
        if (row < 0) return;
        QTableWidgetItem *idItem = d->table->item(row, 0);
        if (!idItem) return;
        const qint64 checkId = idItem->text().toLongLong();
        if (checkId <= 0) return;
        CheckDetailsDialog dlg(this, d->core, checkId);
        dlg.exec();
    });
    d->table->setColumnWidth(0, 60);
    d->table->setColumnWidth(1, 80);
    d->table->setColumnWidth(2, 90);
    d->table->setColumnWidth(3, 80);

    d->totalLabel = new QLabel(this);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel(tr("Дата:")));
    topLayout->addWidget(d->dateEdit);
    topLayout->addStretch();

    auto *btnRefresh = new QPushButton(tr("Обновить"));
    connect(btnRefresh, &QPushButton::clicked, this, &CheckHistoryDialog::refreshTable);
    topLayout->addWidget(btnRefresh);

    auto *btnExport = new QPushButton(tr("Экспорт в CSV"));
    connect(btnExport, &QPushButton::clicked, this, &CheckHistoryDialog::onExportCsv);
    topLayout->addWidget(btnExport);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(topLayout);
    layout->addWidget(d->table);
    layout->addWidget(d->totalLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    refreshTable();
}

CheckHistoryDialog::~CheckHistoryDialog()
{
    delete d;
}

void CheckHistoryDialog::onDateChanged()
{
    refreshTable();
}

void CheckHistoryDialog::refreshTable()
{
    d->table->setRowCount(0);
    d->totalLabel->clear();

    if (!d->core || !d->core->getDatabaseConnection()) return;
    auto *sm = d->core->createSalesManager(this);
    if (!sm) return;

    const QDate date = d->dateEdit->date();
    const QList<Check> checks = sm->getChecks(date, date, -1, false);

    double dayTotal = 0;
    for (const Check &ch : checks) {
        const int row = d->table->rowCount();
        d->table->insertRow(row);
        d->table->setItem(row, 0, new QTableWidgetItem(QString::number(ch.id)));
        d->table->setItem(row, 1, new QTableWidgetItem(ch.time.toString(Qt::ISODate)));
        const double pay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);
        d->table->setItem(row, 2, new QTableWidgetItem(QString::number(pay, 'f', 2)));
        d->table->setItem(row, 3, new QTableWidgetItem(QString::number(ch.discountAmount, 'f', 2)));
        d->table->setItem(row, 4, new QTableWidgetItem(ch.employeeName));
        dayTotal += pay;
    }

    d->totalLabel->setText(tr("Чеков: %1 | Итого за день: %2 ₽")
        .arg(checks.size())
        .arg(QString::number(dayTotal, 'f', 2)));
}

void CheckHistoryDialog::onExportCsv()
{
    if (d->table->rowCount() == 0) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    const QDate date = d->dateEdit->date();
    QString defaultName = tr("Чеки_%1.csv").arg(date.toString(Qt::ISODate));
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
    out << tr("№;Время;Сумма;Скидка;Сотрудник\n");
    for (int row = 0; row < d->table->rowCount(); ++row) {
        out << d->table->item(row, 0)->text() << ";"
            << d->table->item(row, 1)->text() << ";"
            << d->table->item(row, 2)->text() << ";"
            << d->table->item(row, 3)->text() << ";"
            << "\"" << d->table->item(row, 4)->text().replace(QLatin1String("\""), QLatin1String("\"\"")) << "\"\n";
    }
    f.close();
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён."));
}
