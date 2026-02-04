#include "recipesdialog.h"
#include "recipeeditdialog.h"
#include "../easyposcore.h"
#include "../production/productionmanager.h"
#include "../production/structures.h"
#include "../RBAC/structures.h"
#include <QSqlQuery>

RecipesDialog::RecipesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Рецепты производства"))
{
    setupColumns({ tr("ID"), tr("Название"), tr("Продукт на выходе"), tr("Выход (порция)") },
                 { 60, 200, 220, 100 });
    loadTable();
}

ProductionManager *RecipesDialog::productionManager() const
{
    return m_core ? m_core->getProductionManager() : nullptr;
}

QList<QPair<qint64, QString>> RecipesDialog::loadGoodsList() const
{
    QList<QPair<qint64, QString>> list;
    if (!m_core || !m_core->getDatabaseConnection())
        return list;
    QSqlQuery q(m_core->getDatabaseConnection()->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id, name FROM goods WHERE (isdeleted IS NULL OR isdeleted = false) AND isactive = true ORDER BY name"));
    if (!q.exec())
        return list;
    while (q.next())
        list.append({ q.value(0).toLongLong(), q.value(1).toString() });
    return list;
}

void RecipesDialog::loadTable(bool includeDeleted)
{
    clearTable();
    ProductionManager *pm = productionManager();
    if (!pm)
        return;

    const QList<ProductionRecipe> recipes = pm->getRecipes(includeDeleted);
    for (const ProductionRecipe &r : recipes) {
        const int row = insertRow();
        setCellText(row, 0, QString::number(r.id));
        setCellText(row, 1, r.name);
        setCellText(row, 2, r.outputGoodName);
        setCellText(row, 3, QString::number(r.outputQuantity));
    }
    updateRecordCount();
}

void RecipesDialog::addRecord()
{
    addOrEditRecipe(0);
}

void RecipesDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0) {
        showWarning(tr("Выберите рецепт для редактирования."));
        return;
    }
    addOrEditRecipe(cellText(row, 0).toLongLong());
}

void RecipesDialog::addOrEditRecipe(qint64 existingId)
{
    ProductionManager *pm = productionManager();
    if (!pm) {
        showError(tr("Менеджер производства не доступен."));
        return;
    }

    const QList<QPair<qint64, QString>> goodsList = loadGoodsList();
    if (goodsList.isEmpty()) {
        showError(tr("Нет товаров. Сначала добавьте товары в справочник."));
        return;
    }

    UserSession session = m_core->getCurrentSession();
    qint64 employeeId = m_core->getEmployeeIdByUserId(session.userId);
    if (employeeId <= 0) {
        showError(tr("Нет привязки пользователя к сотруднику."));
        return;
    }

    RecipeEditDialog dlg(this);
    dlg.setGoodsList(goodsList);

    if (existingId > 0) {
        ProductionRecipe recipe = pm->getRecipe(existingId);
        if (recipe.id == 0) {
            showError(tr("Рецепт не найден."));
            return;
        }
        dlg.setRecipe(recipe);
        dlg.setComponents(pm->getRecipeComponents(existingId));
        dlg.setWindowTitle(tr("Редактирование рецепта"));
    } else {
        dlg.setWindowTitle(tr("Новый рецепт"));
    }

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString name = dlg.recipeName();
    const qint64 outputGoodId = dlg.outputGoodId();
    const double outputQty = dlg.outputQuantity();
    const QList<QPair<qint64, double>> components = dlg.components();

    QString err;
    if (existingId > 0) {
        if (!pm->updateRecipe(existingId, name, outputGoodId, outputQty, &err)) {
            showError(tr("Ошибка сохранения: %1").arg(err));
            return;
        }
        // Update components: replace all
        for (const RecipeComponent &c : pm->getRecipeComponents(existingId)) {
            pm->removeRecipeComponent(c.id, &err);
        }
        for (const auto &p : components) {
            pm->addRecipeComponent(existingId, p.first, p.second, &err);
        }
        showInfo(tr("Рецепт обновлён."));
    } else {
        const qint64 newId = pm->createRecipe(name, outputGoodId, outputQty, employeeId, &err);
        if (newId <= 0) {
            showError(tr("Ошибка добавления: %1").arg(err));
            return;
        }
        for (const auto &p : components) {
            pm->addRecipeComponent(newId, p.first, p.second, &err);
        }
        showInfo(tr("Рецепт добавлен."));
    }
    loadTable();
}

void RecipesDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;

    const qint64 id = cellText(row, 0).toLongLong();
    const QString name = cellText(row, 1);
    if (!askConfirmation(tr("Удалить рецепт «%1»?").arg(name)))
        return;

    ProductionManager *pm = productionManager();
    if (!pm)
        return;

    QString err;
    if (!pm->deleteRecipe(id, &err)) {
        showError(tr("Ошибка: %1").arg(err));
        return;
    }
    showInfo(tr("Рецепт удалён."));
    loadTable();
}
