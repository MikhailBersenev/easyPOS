#include "productionhistorydialog.h"
#include "ui_productionhistorydialog.h"
#include "../easyposcore.h"
#include "../production/productionmanager.h"
#include "../production/structures.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QStringConverter>
#include <QVariant>

ProductionHistoryDialog::ProductionHistoryDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::ProductionHistoryDialog)
    , m_core(core)
{
    ui->setupUi(this);
    QDate today = QDate::currentDate();
    ui->dateFromEdit->setDate(today.addDays(-30));
    ui->dateToEdit->setDate(today);
    ui->table->horizontalHeader()->setStretchLastSection(true);
    ui->table->setAlternatingRowColors(true);
    ui->table->setColumnWidth(0, 60);
    ui->table->setColumnWidth(1, 180);
    ui->table->setColumnWidth(2, 70);
    ui->table->setColumnWidth(3, 100);
    ui->table->setColumnWidth(4, 140);
    ui->table->setColumnWidth(5, 80);
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(3, Qt::DescendingOrder);
    connect(ui->dateFromEdit, &QDateEdit::dateChanged, this, &ProductionHistoryDialog::onDateRangeChanged);
    connect(ui->dateToEdit, &QDateEdit::dateChanged, this, &ProductionHistoryDialog::onDateRangeChanged);
    connect(ui->recipeCombo, &QComboBox::currentIndexChanged, this, &ProductionHistoryDialog::refreshTable);
    connect(ui->refreshButton, &QPushButton::clicked, this, &ProductionHistoryDialog::refreshTable);
    connect(ui->exportButton, &QPushButton::clicked, this, &ProductionHistoryDialog::onExportCsv);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    ProductionManager *pm = m_core ? m_core->getProductionManager() : nullptr;
    if (pm) {
        ui->recipeCombo->blockSignals(true);
        ui->recipeCombo->addItem(tr("Все рецепты"), QVariant::fromValue(qint64(-1)));
        const QList<ProductionRecipe> recipes = pm->getRecipes(false);
        for (const ProductionRecipe &r : recipes)
            ui->recipeCombo->addItem(r.name, QVariant::fromValue(r.id));
        ui->recipeCombo->blockSignals(false);
    }
    refreshTable();
}

ProductionHistoryDialog::~ProductionHistoryDialog()
{
    delete ui;
}

void ProductionHistoryDialog::onDateRangeChanged()
{
    refreshTable();
}

void ProductionHistoryDialog::refreshTable()
{
    ui->totalLabel->clear();
    if (!m_core) {
        ui->table->setRowCount(0);
        return;
    }
    ProductionManager *pm = m_core->getProductionManager();
    if (!pm) {
        ui->table->setRowCount(0);
        return;
    }

    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    if (from > to) {
        ui->table->setRowCount(0);
        ui->totalLabel->setText(tr("Укажите корректный период."));
        return;
    }

    const qint64 recipeId = ui->recipeCombo->currentData().toLongLong();
    m_lastRuns = pm->getProductionRuns(from, to, recipeId > 0 ? recipeId : -1, false);

    ui->table->setSortingEnabled(false);
    ui->table->setRowCount(0);
    double totalQty = 0;
    for (const ProductionRun &run : m_lastRuns) {
        const int row = ui->table->rowCount();
        ui->table->insertRow(row);
        ui->table->setItem(row, 0, new QTableWidgetItem(QString::number(run.id)));
        ui->table->setItem(row, 1, new QTableWidgetItem(run.recipeName));
        ui->table->setItem(row, 2, new QTableWidgetItem(QString::number(run.quantityProduced, 'f', 2)));
        ui->table->setItem(row, 3, new QTableWidgetItem(run.productionDate.toString(Qt::ISODate)));
        ui->table->setItem(row, 4, new QTableWidgetItem(run.employeeName));
        ui->table->setItem(row, 5, new QTableWidgetItem(run.outputBatchNumber));
        totalQty += run.quantityProduced;
    }
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(3, Qt::DescendingOrder);

    ui->totalLabel->setText(tr("Актов: %1 | Всего произведено (ед.): %2")
        .arg(m_lastRuns.size())
        .arg(QString::number(totalQty, 'f', 2)));
}

void ProductionHistoryDialog::onExportCsv()
{
    if (ui->table->rowCount() == 0) {
        QMessageBox::information(this, windowTitle(), tr("Нет данных для экспорта."));
        return;
    }
    const QDate from = ui->dateFromEdit->date();
    const QDate to = ui->dateToEdit->date();
    QString defaultName = tr("Производства_%1_%2.csv").arg(from.toString(Qt::ISODate)).arg(to.toString(Qt::ISODate));
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
    out << tr("№;Рецепт;Кол-во;Дата;Сотрудник;Номер партии\n");
    for (int row = 0; row < ui->table->rowCount(); ++row) {
        out << ui->table->item(row, 0)->text() << ";"
            << "\"" << ui->table->item(row, 1)->text().replace(QLatin1String("\""), QLatin1String("\"\"")) << "\";"
            << ui->table->item(row, 2)->text() << ";"
            << ui->table->item(row, 3)->text() << ";"
            << "\"" << ui->table->item(row, 4)->text().replace(QLatin1String("\""), QLatin1String("\"\"")) << "\";"
            << ui->table->item(row, 5)->text() << "\n";
    }
    f.close();
    QMessageBox::information(this, windowTitle(), tr("Файл сохранён."));
}
