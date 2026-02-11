#include "productionhistorydialog.h"
#include "ui_productionhistorydialog.h"
#include "reportwizarddialog.h"
#include "../easyposcore.h"
#include "../production/productionmanager.h"
#include "../production/structures.h"
#include <QHeaderView>
#include <QMessageBox>
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
    ui->table->horizontalHeader()->setStretchLastSection(false);
    ui->table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->table->setAlternatingRowColors(true);
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(3, Qt::DescendingOrder);
    connect(ui->dateFromEdit, &QDateEdit::dateChanged, this, &ProductionHistoryDialog::onDateRangeChanged);
    connect(ui->dateToEdit, &QDateEdit::dateChanged, this, &ProductionHistoryDialog::onDateRangeChanged);
    connect(ui->recipeCombo, &QComboBox::currentIndexChanged, this, &ProductionHistoryDialog::refreshTable);
    connect(ui->refreshButton, &QPushButton::clicked, this, &ProductionHistoryDialog::refreshTable);
    connect(ui->exportButton, &QPushButton::clicked, this, &ProductionHistoryDialog::onReportWizard);
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

void ProductionHistoryDialog::onReportWizard()
{
    if (!m_core) return;
    QDate from = ui->dateFromEdit->date();
    QDate to = ui->dateToEdit->date();
    if (from > to) to = from;
    ReportWizardDialog dlg(this, m_core);
    dlg.setPreset(ReportProduction, from, to, 0, 2);
    dlg.exec();
}
