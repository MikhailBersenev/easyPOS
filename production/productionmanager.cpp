#include "productionmanager.h"
#include "../sales/stockmanager.h"
#include "../sales/structures.h"
#include "../logging/logmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDateTime>

ProductionManager::ProductionManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "ProductionManager::ProductionManager";
}

ProductionManager::~ProductionManager()
{
    qDebug() << "ProductionManager::~ProductionManager";
}

void ProductionManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    qDebug() << "ProductionManager::setDatabaseConnection" << (dbConnection ? "set" : "null");
    m_dbConnection = dbConnection;
}

void ProductionManager::setStockManager(StockManager *stockManager)
{
    qDebug() << "ProductionManager::setStockManager" << (stockManager ? "set" : "null");
    m_stockManager = stockManager;
}

bool ProductionManager::dbOk() const
{
    return m_dbConnection && m_dbConnection->isConnected();
}

QList<ProductionRecipe> ProductionManager::getRecipes(bool includeDeleted)
{
    qDebug() << "ProductionManager::getRecipes" << "includeDeleted=" << includeDeleted;
    QList<ProductionRecipe> list;
    if (!dbOk()) {
        qDebug() << "ProductionManager::getRecipes: branch no_db";
        return list;
    }

    QString sql = QStringLiteral(
        "SELECT r.id, r.name, r.outputgoodid, g.name AS output_good_name, "
        "r.outputquantity, r.updatedate, r.employeeid, r.isdeleted "
        "FROM productionrecipes r "
        "JOIN goods g ON g.id = r.outputgoodid ");
    if (!includeDeleted) {
        qDebug() << "ProductionManager::getRecipes: branch exclude_deleted";
        sql += QStringLiteral("WHERE (r.isdeleted IS NULL OR r.isdeleted = false) ");
    }
    sql += QStringLiteral("ORDER BY r.name");

    QSqlQuery q(m_dbConnection->getDatabase());
    if (!q.exec(sql)) {
        qDebug() << "ProductionManager::getRecipes: branch query_failed" << q.lastError().text();
        return list;
    }

    while (q.next()) {
        ProductionRecipe r;
        r.id = q.value(0).toLongLong();
        r.name = q.value(1).toString();
        r.outputGoodId = q.value(2).toLongLong();
        r.outputGoodName = q.value(3).toString();
        r.outputQuantity = q.value(4).toDouble();
        r.updateDate = q.value(5).toDate();
        r.employeeId = q.value(6).toLongLong();
        r.isDeleted = q.value(7).toBool();
        list.append(r);
    }
    return list;
}

ProductionRecipe ProductionManager::getRecipe(qint64 recipeId)
{
    qDebug() << "ProductionManager::getRecipe" << "recipeId=" << recipeId;
    ProductionRecipe r;
    if (!dbOk() || recipeId <= 0) {
        qDebug() << "ProductionManager::getRecipe: branch no_db_or_bad_id";
        return r;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT r.id, r.name, r.outputgoodid, g.name, r.outputquantity, "
        "r.updatedate, r.employeeid, r.isdeleted "
        "FROM productionrecipes r JOIN goods g ON g.id = r.outputgoodid WHERE r.id = :id"));
    q.bindValue(QStringLiteral(":id"), recipeId);
    if (!q.exec() || !q.next()) {
        qDebug() << "ProductionManager::getRecipe: branch no_row" << q.lastError().text();
        return r;
    }

    r.id = q.value(0).toLongLong();
    r.name = q.value(1).toString();
    r.outputGoodId = q.value(2).toLongLong();
    r.outputGoodName = q.value(3).toString();
    r.outputQuantity = q.value(4).toDouble();
    r.updateDate = q.value(5).toDate();
    r.employeeId = q.value(6).toLongLong();
    r.isDeleted = q.value(7).toBool();
    return r;
}

qint64 ProductionManager::createRecipe(const QString &name, qint64 outputGoodId, double outputQuantity,
                                       qint64 employeeId, QString *outError)
{
    qDebug() << "ProductionManager::createRecipe" << "name=" << name << "outputGoodId=" << outputGoodId;
    if (!dbOk()) {
        qDebug() << "ProductionManager::createRecipe: branch no_db";
        if (outError) *outError = QStringLiteral("Нет подключения к БД");
        return 0;
    }
    if (name.trimmed().isEmpty()) {
        qDebug() << "ProductionManager::createRecipe: branch empty_name";
        if (outError) *outError = QStringLiteral("Название рецепта не задано");
        return 0;
    }
    if (outputGoodId <= 0 || outputQuantity <= 0) {
        qDebug() << "ProductionManager::createRecipe: branch bad_output";
        if (outError) *outError = QStringLiteral("Укажите товар на выходе и количество");
        return 0;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO productionrecipes (name, outputgoodid, outputquantity, updatedate, employeeid, isdeleted) "
        "VALUES (:name, :goodid, :qty, :updatedate, :empid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":name"), name.trimmed());
    q.bindValue(QStringLiteral(":goodid"), outputGoodId);
    q.bindValue(QStringLiteral(":qty"), outputQuantity);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":empid"), employeeId);
    if (!q.exec()) {
        qDebug() << "ProductionManager::createRecipe: branch insert_failed" << q.lastError().text();
        m_lastError = q.lastError();
        if (outError) *outError = m_lastError.text();
        return 0;
    }
    if (q.next()) {
        qDebug() << "ProductionManager::createRecipe: branch success";
        const qint64 newId = q.value(0).toLongLong();
        emit recipeAdded(newId);
        return newId;
    }
    return 0;
}

bool ProductionManager::updateRecipe(qint64 recipeId, const QString &name, qint64 outputGoodId,
                                     double outputQuantity, QString *outError)
{
    if (!dbOk() || recipeId <= 0) {
        if (outError) *outError = QStringLiteral("Нет подключения или неверный ID");
        return false;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "UPDATE productionrecipes SET name = :name, outputgoodid = :goodid, "
        "outputquantity = :qty, updatedate = :updatedate "
        "WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"), name.trimmed());
    q.bindValue(QStringLiteral(":goodid"), outputGoodId);
    q.bindValue(QStringLiteral(":qty"), outputQuantity);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":id"), recipeId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        if (outError) *outError = m_lastError.text();
        return false;
    }
    emit recipeUpdated(recipeId);
    return true;
}

bool ProductionManager::deleteRecipe(qint64 recipeId, QString *outError)
{
    if (!dbOk() || recipeId <= 0) {
        if (outError) *outError = QStringLiteral("Нет подключения или неверный ID");
        return false;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "UPDATE productionrecipes SET isdeleted = true, updatedate = :updatedate WHERE id = :id"));
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":id"), recipeId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        if (outError) *outError = m_lastError.text();
        return false;
    }
    emit recipeDeleted(recipeId);
    return true;
}

QList<RecipeComponent> ProductionManager::getRecipeComponents(qint64 recipeId)
{
    QList<RecipeComponent> list;
    if (!dbOk() || recipeId <= 0)
        return list;

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT c.id, c.recipeid, c.goodid, g.name, c.quantity "
        "FROM productionrecipecomponents c JOIN goods g ON g.id = c.goodid WHERE c.recipeid = :rid ORDER BY g.name"));
    q.bindValue(QStringLiteral(":rid"), recipeId);
    if (!q.exec())
        return list;

    while (q.next()) {
        RecipeComponent c;
        c.id = q.value(0).toLongLong();
        c.recipeId = q.value(1).toLongLong();
        c.goodId = q.value(2).toLongLong();
        c.goodName = q.value(3).toString();
        c.quantity = q.value(4).toDouble();
        list.append(c);
    }
    return list;
}

bool ProductionManager::addRecipeComponent(qint64 recipeId, qint64 goodId, double quantity, QString *outError)
{
    if (!dbOk() || recipeId <= 0) {
        if (outError) *outError = QStringLiteral("Нет подключения или неверный рецепт");
        return false;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO productionrecipecomponents (recipeid, goodid, quantity) VALUES (:rid, :gid, :qty)"));
    q.bindValue(QStringLiteral(":rid"), recipeId);
    q.bindValue(QStringLiteral(":gid"), goodId);
    q.bindValue(QStringLiteral(":qty"), quantity);
    if (!q.exec()) {
        m_lastError = q.lastError();
        if (outError) *outError = m_lastError.text();
        return false;
    }
    return true;
}

bool ProductionManager::updateRecipeComponent(qint64 componentId, double quantity, QString *outError)
{
    if (!dbOk() || componentId <= 0) {
        if (outError) *outError = QStringLiteral("Нет подключения или неверный ID");
        return false;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE productionrecipecomponents SET quantity = :qty WHERE id = :id"));
    q.bindValue(QStringLiteral(":qty"), quantity);
    q.bindValue(QStringLiteral(":id"), componentId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        if (outError) *outError = m_lastError.text();
        return false;
    }
    return true;
}

bool ProductionManager::removeRecipeComponent(qint64 componentId, QString *outError)
{
    if (!dbOk() || componentId <= 0) {
        if (outError) *outError = QStringLiteral("Нет подключения или неверный ID");
        return false;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("DELETE FROM productionrecipecomponents WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), componentId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        if (outError) *outError = m_lastError.text();
        return false;
    }
    return true;
}

qint64 ProductionManager::createOutputBatch(qint64 goodId, qint64 quantity, double price, qint64 employeeId)
{
    if (!dbOk())
        return 0;

    QDate today = QDate::currentDate();
    QString batchNumber = QStringLiteral("PROD-%1-%2").arg(goodId).arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss")));

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO batches (goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price, reservedquantity) "
        "VALUES (:goodid, :qnt, :batchnumber, :proddate, :expdate, false, :updatedate, :empid, false, :price, 0) RETURNING id"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    q.bindValue(QStringLiteral(":qnt"), quantity);
    q.bindValue(QStringLiteral(":batchnumber"), batchNumber);
    q.bindValue(QStringLiteral(":proddate"), today);
    q.bindValue(QStringLiteral(":expdate"), today.addYears(1));
    q.bindValue(QStringLiteral(":updatedate"), today);
    q.bindValue(QStringLiteral(":empid"), employeeId);
    q.bindValue(QStringLiteral(":price"), price);
    if (!q.exec() || !q.next())
        return 0;
    return q.value(0).toLongLong();
}

ProductionRunResult ProductionManager::runProduction(qint64 recipeId, double quantity, qint64 employeeId,
                                                    double outputPrice)
{
    ProductionRunResult result;
    if (!dbOk()) {
        result.message = QStringLiteral("Нет подключения к БД");
        return result;
    }
    if (!m_stockManager) {
        result.message = QStringLiteral("Не настроен менеджер склада");
        return result;
    }
    if (quantity <= 0) {
        result.message = QStringLiteral("Количество должно быть больше нуля");
        return result;
    }

    ProductionRecipe recipe = getRecipe(recipeId);
    if (recipe.id == 0) {
        result.message = QStringLiteral("Рецепт не найден");
        return result;
    }

    QList<RecipeComponent> components = getRecipeComponents(recipeId);
    if (recipe.outputQuantity <= 0) {
        result.message = QStringLiteral("Некорректный выход рецепта");
        return result;
    }
    double factor = quantity / recipe.outputQuantity;

    // Проверка наличия сырья и списание
    for (const RecipeComponent &c : components) {
        double required = c.quantity * factor;
        if (required <= 0)
            continue;
        Stock stock = m_stockManager->getStockByGoodId(static_cast<int>(c.goodId));
        if (!stock.hasEnough(required)) {
            result.message = QStringLiteral("Недостаточно сырья «%1»: нужно %2, доступно %3")
                                 .arg(c.goodName).arg(required).arg(stock.availableQuantity());
            return result;
        }
    }

    for (const RecipeComponent &c : components) {
        double toRemove = c.quantity * factor;
        if (toRemove <= 0)
            continue;
        StockOperationResult sr = m_stockManager->removeStock(static_cast<int>(c.goodId), toRemove);
        if (!sr.success) {
            result.message = QStringLiteral("Ошибка списания «%1»: %2").arg(c.goodName, sr.message);
            return result;
        }
    }

    qint64 outputQnt = static_cast<qint64>(quantity);
    if (outputQnt < 1)
        outputQnt = 1;
    qint64 batchId = createOutputBatch(recipe.outputGoodId, outputQnt, outputPrice, employeeId);
    if (batchId == 0) {
        result.message = QStringLiteral("Не удалось создать партию готовой продукции");
        // Откат списания сырья вручную не делаем для простоты; в реальности лучше транзакция
        return result;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO productionruns (recipeid, quantityproduced, productiondate, employeeid, outputbatchid, isdeleted) "
        "VALUES (:rid, :qty, :pdate, :empid, :batchid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":rid"), recipeId);
    q.bindValue(QStringLiteral(":qty"), quantity);
    q.bindValue(QStringLiteral(":pdate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":empid"), employeeId);
    q.bindValue(QStringLiteral(":batchid"), batchId);
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError();
        result.message = QStringLiteral("Ошибка записи акта производства: %1").arg(m_lastError.text());
        return result;
    }

    result.success = true;
    result.runId = q.value(0).toLongLong();
    result.outputBatchId = batchId;
    result.message = QStringLiteral("Производство проведено. Партия %1.").arg(batchId);
    emit productionCompleted(result.runId, result.outputBatchId);
    return result;
}

QList<ProductionRun> ProductionManager::getProductionRuns(const QDate &from, const QDate &to,
                                                           qint64 recipeId, bool includeDeleted)
{
    QList<ProductionRun> list;
    if (!dbOk())
        return list;

    QString sql = QStringLiteral(
        "SELECT pr.id, pr.recipeid, r.name AS recipe_name, pr.quantityproduced, pr.productiondate, "
        "pr.employeeid, trim(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'') || ' ' || COALESCE(e.middlename,'')) AS employee_name, pr.outputbatchid, COALESCE(b.batchnumber,'') AS batch_number, pr.isdeleted "
        "FROM productionruns pr "
        "JOIN productionrecipes r ON r.id = pr.recipeid "
        "LEFT JOIN employees e ON e.id = pr.employeeid "
        "LEFT JOIN batches b ON b.id = pr.outputbatchid "
        "WHERE pr.productiondate BETWEEN :dfrom AND :dto ");
    if (recipeId > 0)
        sql += QStringLiteral("AND pr.recipeid = :rid ");
    if (!includeDeleted)
        sql += QStringLiteral("AND (pr.isdeleted IS NULL OR pr.isdeleted = false) ");
    sql += QStringLiteral("ORDER BY pr.productiondate DESC, pr.id DESC");

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":dfrom"), from);
    q.bindValue(QStringLiteral(":dto"), to);
    if (recipeId > 0)
        q.bindValue(QStringLiteral(":rid"), recipeId);
    if (!q.exec())
        return list;

    while (q.next()) {
        ProductionRun run;
        run.id = q.value(0).toLongLong();
        run.recipeId = q.value(1).toLongLong();
        run.recipeName = q.value(2).toString();
        run.quantityProduced = q.value(3).toDouble();
        run.productionDate = q.value(4).toDate();
        run.employeeId = q.value(5).toLongLong();
        run.employeeName = q.value(6).toString();
        run.outputBatchId = q.value(7).toLongLong();
        run.outputBatchNumber = q.value(8).toString();
        run.isDeleted = q.value(9).toBool();
        list.append(run);
    }
    return list;
}

QSqlError ProductionManager::lastError() const
{
    return m_lastError;
}
