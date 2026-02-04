#ifndef PRODUCTIONMANAGER_H
#define PRODUCTIONMANAGER_H

#include <QObject>
#include <QList>
#include <QSqlError>
#include "../db/databaseconnection.h"
#include "structures.h"

class StockManager;

class ProductionManager : public QObject
{
    Q_OBJECT
public:
    explicit ProductionManager(QObject *parent = nullptr);
    ~ProductionManager();

    void setDatabaseConnection(DatabaseConnection *dbConnection);
    void setStockManager(StockManager *stockManager);

    // Рецепты
    QList<ProductionRecipe> getRecipes(bool includeDeleted = false);
    ProductionRecipe getRecipe(qint64 recipeId);
    qint64 createRecipe(const QString &name, qint64 outputGoodId, double outputQuantity,
                        qint64 employeeId, QString *outError = nullptr);
    bool updateRecipe(qint64 recipeId, const QString &name, qint64 outputGoodId,
                      double outputQuantity, QString *outError = nullptr);
    bool deleteRecipe(qint64 recipeId, QString *outError = nullptr);

    // Компоненты рецепта
    QList<RecipeComponent> getRecipeComponents(qint64 recipeId);
    bool addRecipeComponent(qint64 recipeId, qint64 goodId, double quantity, QString *outError = nullptr);
    bool updateRecipeComponent(qint64 componentId, double quantity, QString *outError = nullptr);
    bool removeRecipeComponent(qint64 componentId, QString *outError = nullptr);

    // Проведение производства
    ProductionRunResult runProduction(qint64 recipeId, double quantity, qint64 employeeId,
                                      double outputPrice = 0.0);

    // История производств
    QList<ProductionRun> getProductionRuns(const QDate &from, const QDate &to,
                                          qint64 recipeId = -1, bool includeDeleted = false);

    QSqlError lastError() const;

signals:
    void recipeAdded(qint64 recipeId);
    void recipeUpdated(qint64 recipeId);
    void recipeDeleted(qint64 recipeId);
    void productionCompleted(qint64 runId, qint64 batchId);
    void operationFailed(const QString &message);

private:
    DatabaseConnection *m_dbConnection = nullptr;
    StockManager *m_stockManager = nullptr;
    QSqlError m_lastError;

    bool dbOk() const;
    qint64 createOutputBatch(qint64 goodId, qint64 quantity, double price, qint64 employeeId);
};

#endif // PRODUCTIONMANAGER_H
