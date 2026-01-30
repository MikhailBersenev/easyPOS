#include "stockmanager.h"
#include "../goods/structures.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDate>
#include <QDebug>

StockManager::StockManager(QObject *parent)
    : QObject(parent)
    , m_dbConnection(nullptr)
{
}

StockManager::~StockManager()
{
}

void StockManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    m_dbConnection = dbConnection;
}

static bool dbOk(DatabaseConnection *c, StockOperationResult &r, QSqlError &lastErr)
{
    if (!c || !c->isConnected()) {
        r.message = QStringLiteral("Нет подключения к базе данных");
        lastErr = QSqlError(QStringLiteral("No database connection"),
                            r.message, QSqlError::ConnectionError);
        r.error = lastErr;
        return false;
    }
    return true;
}

int StockManager::getGoodIdByName(const QString &goodName)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return 0;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT id FROM goods WHERE name = :name AND (isdeleted IS NULL OR isdeleted = false)"));
    q.bindValue(QStringLiteral(":name"), goodName);
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

bool StockManager::goodExists(int goodId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return false;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT id FROM goods WHERE id = :id AND (isdeleted IS NULL OR isdeleted = false)"));
    q.bindValue(QStringLiteral(":id"), goodId);
    return q.exec() && q.next();
}

Stock StockManager::getStockByGoodId(int goodId, bool includeDeleted)
{
    Stock stock;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return stock;
    QSqlQuery q(m_dbConnection->getDatabase());
    QString sql = QStringLiteral(
        "SELECT g.id AS goodid, g.name AS goodname, "
        "COALESCE(SUM(b.qnt), 0) AS quantity, "
        "COALESCE(SUM(b.reservedquantity), 0) AS reservedquantity, "
        "MAX(b.updatedate) AS updatedate "
        "FROM goods g "
        "LEFT JOIN batches b ON b.goodid = g.id AND (b.isdeleted IS NULL OR b.isdeleted = false) "
        "WHERE g.id = :goodid AND (g.isdeleted IS NULL OR g.isdeleted = false) "
        "GROUP BY g.id, g.name");
    q.prepare(sql);
    q.bindValue(QStringLiteral(":goodid"), goodId);
    if (q.exec() && q.next()) {
        stock.id = goodId;
        stock.goodId = q.value(QStringLiteral("goodid")).toInt();
        stock.goodName = q.value(QStringLiteral("goodname")).toString();
        stock.quantity = q.value(QStringLiteral("quantity")).toDouble();
        stock.reservedQuantity = q.value(QStringLiteral("reservedquantity")).toDouble();
        stock.updateDate = q.value(QStringLiteral("updatedate")).toDateTime();
        stock.isDeleted = false;
    }
    return stock;
}

Stock StockManager::getStockByGoodName(const QString &goodName, bool includeDeleted)
{
    Q_UNUSED(includeDeleted);
    Stock stock;
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0)
        return stock;
    return getStockByGoodId(goodId);
}

Stock StockManager::getStock(int stockId, bool includeDeleted)
{
    Q_UNUSED(includeDeleted);
    return getStockByGoodId(stockId);
}

QList<Stock> StockManager::getAllStocks(bool includeDeleted)
{
    Q_UNUSED(includeDeleted);
    QList<Stock> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;
    QSqlQuery q(m_dbConnection->getDatabase());
    QString sql = QStringLiteral(
        "SELECT g.id AS goodid, g.name AS goodname, "
        "COALESCE(SUM(b.qnt), 0) AS quantity, "
        "COALESCE(SUM(b.reservedquantity), 0) AS reservedquantity, "
        "MAX(b.updatedate) AS updatedate "
        "FROM goods g "
        "LEFT JOIN batches b ON b.goodid = g.id AND (b.isdeleted IS NULL OR b.isdeleted = false) "
        "WHERE (g.isdeleted IS NULL OR g.isdeleted = false) "
        "GROUP BY g.id, g.name "
        "HAVING COALESCE(SUM(b.qnt), 0) > 0 OR COALESCE(SUM(b.reservedquantity), 0) > 0 "
        "ORDER BY g.name");
    if (!q.exec(sql))
        return list;
    while (q.next()) {
        Stock s;
        s.id = q.value(QStringLiteral("goodid")).toInt();
        s.goodId = s.id;
        s.goodName = q.value(QStringLiteral("goodname")).toString();
        s.quantity = q.value(QStringLiteral("quantity")).toDouble();
        s.reservedQuantity = q.value(QStringLiteral("reservedquantity")).toDouble();
        s.updateDate = q.value(QStringLiteral("updatedate")).toDateTime();
        s.isDeleted = false;
        list.append(s);
    }
    return list;
}

QList<Stock> StockManager::getStocksByGoodId(int goodId, bool includeDeleted)
{
    Q_UNUSED(includeDeleted);
    QList<Stock> list;
    Stock s = getStockByGoodId(goodId);
    if (s.id > 0)
        list.append(s);
    return list;
}

bool StockManager::hasEnoughStock(int goodId, double requiredQuantity)
{
    Stock s = getStockByGoodId(goodId);
    return s.hasEnough(requiredQuantity);
}

bool StockManager::hasEnoughStock(const QString &goodName, double requiredQuantity)
{
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0)
        return false;
    return hasEnoughStock(goodId, requiredQuantity);
}

bool StockManager::stockExists(int stockId, bool includeDeleted)
{
    Q_UNUSED(includeDeleted);
    Stock s = getStockByGoodId(stockId);
    return s.id > 0 && s.quantity > 0;
}

bool StockManager::stockExistsByGoodId(int goodId, bool includeDeleted)
{
    return stockExists(goodId, includeDeleted);
}

StockOperationResult StockManager::setStock(int goodId, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (!goodExists(goodId)) {
        r.message = QStringLiteral("Товар не найден");
        return r;
    }
    if (quantity < 0) {
        r.message = QStringLiteral("Количество не может быть отрицательным");
        return r;
    }
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE batches SET isdeleted = true WHERE goodid = :goodid"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при удалении старых партий: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    if (quantity > 0) {
        QDate today = QDate::currentDate();
        q.prepare(QStringLiteral(
            "INSERT INTO batches (goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price, reservedquantity) "
            "VALUES (:goodid, :qnt, :batchnumber, :proddate, :expdate, false, :updatedate, 1, false, 0, 0) RETURNING id"));
        q.bindValue(QStringLiteral(":goodid"), goodId);
        q.bindValue(QStringLiteral(":qnt"), static_cast<qint64>(quantity));
        q.bindValue(QStringLiteral(":batchnumber"), QStringLiteral("STOCK-%1").arg(goodId));
        q.bindValue(QStringLiteral(":proddate"), today);
        q.bindValue(QStringLiteral(":expdate"), today.addYears(1));
        q.bindValue(QStringLiteral(":updatedate"), today);
        if (!q.exec()) {
            m_lastError = q.lastError();
            r.error = m_lastError;
            r.message = QStringLiteral("Ошибка при создании партии: %1").arg(m_lastError.text());
            emit operationFailed(r.message);
            return r;
        }
        if (q.next())
            r.stockId = q.value(0).toInt();
    }
    r.success = true;
    r.stockId = goodId;
    r.message = QStringLiteral("Остаток установлен");
    emit stockUpdated(goodId, goodId, quantity);
    return r;
}

StockOperationResult StockManager::setStock(const QString &goodName, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0) {
        r.message = QStringLiteral("Товар '%1' не найден").arg(goodName);
        return r;
    }
    return setStock(goodId, quantity);
}

StockOperationResult StockManager::addStock(int goodId, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (!goodExists(goodId)) {
        r.message = QStringLiteral("Товар не найден");
        return r;
    }
    if (quantity <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }
    QDate today = QDate::currentDate();
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO batches (goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price, reservedquantity) "
        "VALUES (:goodid, :qnt, :batchnumber, :proddate, :expdate, false, :updatedate, 1, false, 0, 0) RETURNING id"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    q.bindValue(QStringLiteral(":qnt"), static_cast<qint64>(quantity));
    q.bindValue(QStringLiteral(":batchnumber"), QStringLiteral("ADD-%1-%2").arg(goodId).arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"))));
    q.bindValue(QStringLiteral(":proddate"), today);
    q.bindValue(QStringLiteral(":expdate"), today.addYears(1));
    q.bindValue(QStringLiteral(":updatedate"), today);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при добавлении партии: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    if (q.next())
        r.stockId = q.value(0).toInt();
    r.success = true;
    r.stockId = goodId;
    r.message = QStringLiteral("Товар добавлен на склад");
    emit stockAdded(goodId, goodId, quantity);
    return r;
}

StockOperationResult StockManager::addStock(const QString &goodName, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0) {
        r.message = QStringLiteral("Товар '%1' не найден").arg(goodName);
        return r;
    }
    return addStock(goodId, quantity);
}

StockOperationResult StockManager::removeStock(int goodId, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (quantity <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }
    Stock s = getStockByGoodId(goodId);
    if (s.id == 0 || !s.hasEnough(quantity)) {
        r.message = QStringLiteral("Недостаточно товара. Доступно: %1, требуется: %2")
                        .arg(s.availableQuantity()).arg(quantity);
        return r;
    }
    double remaining = quantity;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id, qnt, reservedquantity FROM batches "
        "WHERE goodid = :goodid AND (isdeleted IS NULL OR isdeleted = false) "
        "ORDER BY proddate ASC"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при получении партий: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    while (q.next() && remaining > 0) {
        qint64 batchId = q.value(0).toLongLong();
        qint64 batchQnt = q.value(1).toLongLong();
        qint64 batchReserved = q.value(2).toLongLong();
        qint64 available = batchQnt - batchReserved;
        if (available <= 0)
            continue;
        qint64 toRemove = qMin(static_cast<qint64>(remaining), available);
        qint64 newQnt = batchQnt - toRemove;
        QSqlQuery u(m_dbConnection->getDatabase());
        u.prepare(QStringLiteral("UPDATE batches SET qnt = :qnt, updatedate = :updatedate WHERE id = :id"));
        u.bindValue(QStringLiteral(":qnt"), newQnt);
        u.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
        u.bindValue(QStringLiteral(":id"), batchId);
        if (!u.exec()) {
            m_lastError = u.lastError();
            r.error = m_lastError;
            r.message = QStringLiteral("Ошибка при списании из партии: %1").arg(m_lastError.text());
            emit operationFailed(r.message);
            return r;
        }
        remaining -= toRemove;
    }
    if (remaining > 0) {
        r.message = QStringLiteral("Не удалось списать полностью. Осталось: %1").arg(remaining);
        return r;
    }
    r.success = true;
    r.stockId = goodId;
    r.message = QStringLiteral("Товар списан");
    emit stockRemoved(goodId, goodId, quantity);
    return r;
}

StockOperationResult StockManager::removeStock(const QString &goodName, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0) {
        r.message = QStringLiteral("Товар '%1' не найден").arg(goodName);
        return r;
    }
    return removeStock(goodId, quantity);
}

StockOperationResult StockManager::reserveStock(int goodId, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (quantity <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }
    Stock s = getStockByGoodId(goodId);
    if (s.id == 0 || !s.hasEnough(quantity)) {
        r.message = QStringLiteral("Недостаточно товара для резервирования. Доступно: %1, требуется: %2")
                        .arg(s.availableQuantity()).arg(quantity);
        return r;
    }
    double remaining = quantity;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id, qnt, reservedquantity FROM batches "
        "WHERE goodid = :goodid AND (isdeleted IS NULL OR isdeleted = false) "
        "ORDER BY proddate ASC"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при получении партий: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    while (q.next() && remaining > 0) {
        qint64 batchId = q.value(0).toLongLong();
        qint64 batchQnt = q.value(1).toLongLong();
        qint64 batchReserved = q.value(2).toLongLong();
        qint64 available = batchQnt - batchReserved;
        if (available <= 0)
            continue;
        qint64 toReserve = qMin(static_cast<qint64>(remaining), available);
        qint64 newReserved = batchReserved + toReserve;
        QSqlQuery u(m_dbConnection->getDatabase());
        u.prepare(QStringLiteral("UPDATE batches SET reservedquantity = :reserved, updatedate = :updatedate WHERE id = :id"));
        u.bindValue(QStringLiteral(":reserved"), newReserved);
        u.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
        u.bindValue(QStringLiteral(":id"), batchId);
        if (!u.exec()) {
            m_lastError = u.lastError();
            r.error = m_lastError;
            r.message = QStringLiteral("Ошибка при резервировании партии: %1").arg(m_lastError.text());
            emit operationFailed(r.message);
            return r;
        }
        remaining -= toReserve;
    }
    if (remaining > 0) {
        r.message = QStringLiteral("Не удалось зарезервировать полностью. Осталось: %1").arg(remaining);
        return r;
    }
    r.success = true;
    r.stockId = goodId;
    r.message = QStringLiteral("Товар зарезервирован");
    emit stockReserved(goodId, goodId, quantity);
    return r;
}

StockOperationResult StockManager::reserveStock(const QString &goodName, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0) {
        r.message = QStringLiteral("Товар '%1' не найден").arg(goodName);
        return r;
    }
    return reserveStock(goodId, quantity);
}

StockOperationResult StockManager::releaseReserve(int goodId, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (quantity <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }
    Stock s = getStockByGoodId(goodId);
    if (s.reservedQuantity < quantity) {
        r.message = QStringLiteral("Недостаточно зарезервированного товара. Зарезервировано: %1, требуется: %2")
                        .arg(s.reservedQuantity).arg(quantity);
        return r;
    }
    double remaining = quantity;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id, reservedquantity FROM batches "
        "WHERE goodid = :goodid AND (isdeleted IS NULL OR isdeleted = false) AND reservedquantity > 0 "
        "ORDER BY proddate ASC"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при получении партий: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    while (q.next() && remaining > 0) {
        qint64 batchId = q.value(0).toLongLong();
        qint64 batchReserved = q.value(1).toLongLong();
        if (batchReserved <= 0)
            continue;
        qint64 toRelease = qMin(static_cast<qint64>(remaining), batchReserved);
        qint64 newReserved = batchReserved - toRelease;
        QSqlQuery u(m_dbConnection->getDatabase());
        u.prepare(QStringLiteral("UPDATE batches SET reservedquantity = :reserved, updatedate = :updatedate WHERE id = :id"));
        u.bindValue(QStringLiteral(":reserved"), newReserved);
        u.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
        u.bindValue(QStringLiteral(":id"), batchId);
        if (!u.exec()) {
            m_lastError = u.lastError();
            r.error = m_lastError;
            r.message = QStringLiteral("Ошибка при снятии резерва партии: %1").arg(m_lastError.text());
            emit operationFailed(r.message);
            return r;
        }
        remaining -= toRelease;
    }
    r.success = true;
    r.stockId = goodId;
    r.message = QStringLiteral("Резерв снят");
    emit stockReleased(goodId, goodId, quantity);
    return r;
}

StockOperationResult StockManager::releaseReserve(const QString &goodName, double quantity)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    int goodId = getGoodIdByName(goodName);
    if (goodId == 0) {
        r.message = QStringLiteral("Товар '%1' не найден").arg(goodName);
        return r;
    }
    return releaseReserve(goodId, quantity);
}

StockOperationResult StockManager::deleteStock(int goodId)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE batches SET isdeleted = true, updatedate = :updatedate WHERE goodid = :goodid"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при удалении партий: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    r.success = true;
    r.stockId = goodId;
    r.message = QStringLiteral("Остаток удален");
    emit stockDeleted(goodId, goodId);
    return r;
}

StockOperationResult StockManager::restoreStock(int stockId)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE batches SET isdeleted = false, updatedate = :updatedate WHERE goodid = :goodid"));
    q.bindValue(QStringLiteral(":goodid"), stockId);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка при восстановлении партий: %1").arg(m_lastError.text());
        emit operationFailed(r.message);
        return r;
    }
    r.success = true;
    r.stockId = stockId;
    r.message = QStringLiteral("Остаток восстановлен");
    emit stockRestored(stockId, stockId);
    return r;
}

StockOperationResult StockManager::setStockDeletedStatus(int goodId, bool isDeleted)
{
    if (isDeleted)
        return deleteStock(goodId);
    return restoreStock(goodId);
}

StockOperationResult StockManager::upsertStock(int goodId, double quantity, bool addToExisting)
{
    if (addToExisting)
        return addStock(goodId, quantity);
    return setStock(goodId, quantity);
}

QSqlError StockManager::lastError() const
{
    return m_lastError;
}
