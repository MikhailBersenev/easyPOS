#include "productionrundialog.h"
#include "ui_productionrundialog.h"
#include "../easyposcore.h"
#include "../production/productionmanager.h"
#include "../production/structures.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include "../RBAC/structures.h"
#include <QMessageBox>

ProductionRunDialog::ProductionRunDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::ProductionRunDialog)
    , m_core(core)
{
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate::currentDate());
    connect(ui->recipeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProductionRunDialog::onRecipeSelected);
    connect(ui->quantitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this] { updateComponentsInfo(); });
    connect(ui->runButton, &QPushButton::clicked, this, &ProductionRunDialog::onRunClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    loadRecipes();
    onRecipeSelected(ui->recipeCombo->currentIndex());
}

ProductionRunDialog::~ProductionRunDialog()
{
    delete ui;
}

void ProductionRunDialog::loadRecipes()
{
    ui->recipeCombo->clear();
    ProductionManager *pm = m_core ? m_core->getProductionManager() : nullptr;
    if (!pm)
        return;
    const QList<ProductionRecipe> recipes = pm->getRecipes(false);
    for (const ProductionRecipe &r : recipes) {
        ui->recipeCombo->addItem(QStringLiteral("%1 → %2").arg(r.name, r.outputGoodName), r.id);
    }
}

void ProductionRunDialog::onRecipeSelected(int index)
{
    Q_UNUSED(index);
    updateComponentsInfo();
}

void ProductionRunDialog::updateComponentsInfo()
{
    const qint64 recipeId = ui->recipeCombo->currentData().toLongLong();
    if (recipeId <= 0) {
        ui->componentsValueLabel->setText(tr("—"));
        return;
    }
    ProductionManager *pm = m_core ? m_core->getProductionManager() : nullptr;
    if (!pm) {
        ui->componentsValueLabel->setText(tr("—"));
        return;
    }
    const double factor = ui->quantitySpin->value();
    const QList<RecipeComponent> components = pm->getRecipeComponents(recipeId);
    ProductionRecipe recipe = pm->getRecipe(recipeId);
    if (recipe.outputQuantity <= 0) {
        ui->componentsValueLabel->setText(tr("—"));
        return;
    }
    const double mult = factor / recipe.outputQuantity;
    QStringList lines;
    for (const RecipeComponent &c : components) {
        const double need = c.quantity * mult;
        lines.append(tr("%1: %2").arg(c.goodName).arg(need));
    }
    ui->componentsValueLabel->setText(lines.isEmpty() ? tr("Нет компонентов") : lines.join(QStringLiteral("\n")));
}

void ProductionRunDialog::onRunClicked()
{
    ProductionManager *pm = m_core ? m_core->getProductionManager() : nullptr;
    if (!pm) {
        QMessageBox::warning(this, windowTitle(), tr("Менеджер производства не доступен."));
        return;
    }

    const qint64 recipeId = ui->recipeCombo->currentData().toLongLong();
    if (recipeId <= 0) {
        QMessageBox::warning(this, windowTitle(), tr("Выберите рецепт."));
        return;
    }

    UserSession session = m_core->getCurrentSession();
    qint64 employeeId = m_core->getEmployeeIdByUserId(session.userId);
    if (employeeId <= 0) {
        QMessageBox::warning(this, windowTitle(), tr("Нет привязки пользователя к сотруднику."));
        return;
    }

    const double quantity = ui->quantitySpin->value();
    const double price = ui->priceSpin->value();

    ProductionRunResult result = pm->runProduction(recipeId, quantity, employeeId, price);
    if (result.success) {
        if (auto *alerts = m_core->createAlertsManager())
            alerts->log(AlertCategory::Production, AlertSignature::ProductionRunCompleted,
                        tr("Производство: рецепт %1, кол-во %2, runId=%3").arg(recipeId).arg(quantity).arg(result.runId),
                        session.userId, employeeId);
        QMessageBox::information(this, windowTitle(), result.message);
        loadRecipes();
        updateComponentsInfo();
    } else {
        QMessageBox::warning(this, windowTitle(), result.message);
    }
}
