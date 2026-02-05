#include "checkhistorydialog.h"
#include "ui_checkhistorydialog.h"
#include "checkdetailsdialog.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include "../shifts/shiftmanager.h"
#include "../shifts/structures.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QLocale>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QStringConverter>
#include <QVariant>

CheckHistoryDialog::CheckHistoryDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::CheckHistoryDialog)
    , m_core(core)
{
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate::currentDate());
    ui->table->horizontalHeader()->setStretchLastSection(true);
    ui->table->setAlternatingRowColors(true);
    ui->table->setColumnWidth(0, 60);
    ui->table->setColumnWidth(1, 80);
    ui->table->setColumnWidth(2, 90);
    ui->table->setColumnWidth(3, 70);
    ui->table->setColumnWidth(4, 60);   // Смена
    ui->table->setColumnWidth(5, 120); // Сотрудник
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(4, Qt::AscendingOrder);  // по смене, затем по времени (порядок из SQL)
    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &CheckHistoryDialog::onDateChanged);
    connect(ui->shiftCombo, &QComboBox::currentIndexChanged, this, &CheckHistoryDialog::refreshTable);
    connect(ui->table, &QTableWidget::doubleClicked, this, [this](const QModelIndex &) {
        const int row = ui->table->currentRow();
        if (row < 0) return;
        QTableWidgetItem *idItem = ui->table->item(row, 0);
        if (!idItem) return;
        const qint64 checkId = idItem->text().toLongLong();
        if (checkId <= 0) return;
        CheckDetailsDialog dlg(this, m_core, checkId);
        dlg.exec();
    });
    connect(ui->refreshButton, &QPushButton::clicked, this, &CheckHistoryDialog::refreshTable);
    connect(ui->exportButton, &QPushButton::clicked, this, &CheckHistoryDialog::onExportCsv);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    refreshTable();
}

CheckHistoryDialog::~CheckHistoryDialog()
{
    delete ui;
}

void CheckHistoryDialog::onDateChanged()
{
    refreshTable();
}

void CheckHistoryDialog::refreshTable()
{
    ui->totalLabel->clear();
    if (!m_core || !m_core->getDatabaseConnection()) {
        ui->table->setRowCount(0);
        return;
    }
    const QDate date = ui->dateEdit->date();

    // Список смен за выбранную дату (сначала комбо, чтобы корректно взять shiftId для getChecks)
    if (auto *shiftMgr = m_core->getShiftManager()) {
        const QList<WorkShift> shifts = shiftMgr->getShifts(0, date, date);
        const qint64 prevShiftId = ui->shiftCombo->currentData().toLongLong();
        ui->shiftCombo->blockSignals(true);
        ui->shiftCombo->clear();
        ui->shiftCombo->addItem(tr("Все смены"), QVariant::fromValue(qint64(-1)));
        for (const WorkShift &ws : shifts) {
            QString label = ws.endedDate.isValid()
                ? tr("№%1 %2–%3").arg(ws.id).arg(ws.startedTime.toString(QStringLiteral("HH:mm"))).arg(ws.endedTime.toString(QStringLiteral("HH:mm")))
                : tr("№%1 %2 …").arg(ws.id).arg(ws.startedTime.toString(QStringLiteral("HH:mm")));
            ui->shiftCombo->addItem(label, QVariant::fromValue(ws.id));
        }
        int idx = ui->shiftCombo->findData(QVariant::fromValue(prevShiftId));
        if (idx < 0)
            idx = 0;
        ui->shiftCombo->setCurrentIndex(idx);
        ui->shiftCombo->blockSignals(false);
    } else {
        ui->shiftCombo->blockSignals(true);
        ui->shiftCombo->clear();
        ui->shiftCombo->addItem(tr("Все смены"), QVariant::fromValue(qint64(-1)));
        ui->shiftCombo->setCurrentIndex(0);
        ui->shiftCombo->blockSignals(false);
    }

    auto *sm = m_core->createSalesManager(this);
    if (!sm) {
        ui->table->setRowCount(0);
        return;
    }

    const qint64 shiftId = ui->shiftCombo->currentData().toLongLong();
    m_lastChecks = sm->getChecks(date, date, -1, false, (shiftId > 0) ? shiftId : qint64(-1));

    ui->table->setSortingEnabled(false);
    ui->table->setRowCount(0);
    double dayTotal = 0;
    for (const Check &ch : m_lastChecks) {
        const int row = ui->table->rowCount();
        ui->table->insertRow(row);
        ui->table->setItem(row, 0, new QTableWidgetItem(QString::number(ch.id)));
        ui->table->setItem(row, 1, new QTableWidgetItem(ch.time.toString(Qt::ISODate)));
        const double pay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);
        ui->table->setItem(row, 2, new QTableWidgetItem(QString::number(pay, 'f', 2)));
        ui->table->setItem(row, 3, new QTableWidgetItem(QString::number(ch.discountAmount, 'f', 2)));
        ui->table->setItem(row, 4, new QTableWidgetItem(ch.shiftId > 0 ? QString::number(ch.shiftId) : QString()));
        ui->table->setItem(row, 5, new QTableWidgetItem(ch.employeeName));
        dayTotal += pay;
    }
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(4, Qt::AscendingOrder);

    ui->totalLabel->setText(tr("Чеков: %1 | Итого за день: %2 ₽")
        .arg(m_lastChecks.size())
        .arg(QString::number(dayTotal, 'f', 2)));
}

void CheckHistoryDialog::onExportCsv()
{
    if (ui->table->rowCount() == 0) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    const QDate date = ui->dateEdit->date();
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
    out << tr("№;Время;Сумма;Скидка;Смена;Сотрудник\n");
    for (int row = 0; row < ui->table->rowCount(); ++row) {
        out << ui->table->item(row, 0)->text() << ";"
            << ui->table->item(row, 1)->text() << ";"
            << ui->table->item(row, 2)->text() << ";"
            << ui->table->item(row, 3)->text() << ";"
            << ui->table->item(row, 4)->text() << ";"
            << "\"" << ui->table->item(row, 5)->text().replace(QLatin1String("\""), QLatin1String("\"\"")) << "\"\n";
    }
    f.close();
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён."));
}
