#ifndef PRODUCTION_STRUCTURES_H
#define PRODUCTION_STRUCTURES_H

#include <QString>
#include <QDate>

// Рецепт производства
struct ProductionRecipe {
    qint64 id = 0;
    QString name;
    qint64 outputGoodId = 0;
    QString outputGoodName;
    double outputQuantity = 1.0;
    QDate updateDate;
    qint64 employeeId = 0;
    bool isDeleted = false;

    ProductionRecipe() = default;
};

// Компонент рецепта (сырьё)
struct RecipeComponent {
    qint64 id = 0;
    qint64 recipeId = 0;
    qint64 goodId = 0;
    QString goodName;
    double quantity = 0.0;

    RecipeComponent() = default;
};

// Акт производства
struct ProductionRun {
    qint64 id = 0;
    qint64 recipeId = 0;
    QString recipeName;
    double quantityProduced = 0.0;
    QDate productionDate;
    qint64 employeeId = 0;
    QString employeeName;
    qint64 outputBatchId = 0;
    bool isDeleted = false;

    ProductionRun() = default;
};

// Результат операции производства
struct ProductionRunResult {
    bool success = false;
    QString message;
    qint64 runId = 0;
    qint64 outputBatchId = 0;
};

#endif // PRODUCTION_STRUCTURES_H
