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
        stock.updateDate = q.value(QStringLiteral("updatedate")).toDate();
        QTime t = q.value(QStringLiteral("updatedate")).toTime();
        stock.updateTime = t.isValid() ? t : QTime(0, 0);
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
        s.updateDate = q.value(QStringLiteral("updatedate")).toDate();
        QTime ut = q.value(QStringLiteral("updatedate")).toTime();
        s.updateTime = ut.isValid() ? ut : QTime(0, 0);
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

QList<BatchDetail> StockManager::getBatches(bool includeDeleted)
{
    QList<BatchDetail> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;
    QString sql = QStringLiteral(
        "SELECT b.id, b.goodid, g.name, COALESCE(c.name,''), COALESCE(b.batchnumber,''), b.qnt, "
        "COALESCE(b.reservedquantity, 0), b.price, b.proddate, b.expdate, "
        "COALESCE(b.writtenoff, false), b.updatedate, b.emoloyeeid, COALESCE(b.isdeleted, false) "
        "FROM batches b JOIN goods g ON g.id = b.goodid "
        "LEFT JOIN goodcats c ON c.id = g.categoryid ");
    if (!includeDeleted)
        sql += QStringLiteral("WHERE (b.isdeleted IS NULL OR b.isdeleted = false) ");
    sql += QStringLiteral("ORDER BY g.name, b.proddate DESC, b.id DESC");
    QSqlQuery q(m_dbConnection->getDatabase());
    if (!q.exec(sql))
        return list;
    while (q.next()) {
        BatchDetail d;
        d.id = q.value(0).toLongLong();
        d.goodId = q.value(1).toLongLong();
        d.goodName = q.value(2).toString();
        d.categoryName = q.value(3).toString();
        d.batchNumber = q.value(4).toString();
        d.qnt = q.value(5).toLongLong();
        d.reservedQuantity = q.value(6).toLongLong();
        d.price = q.value(7).toDouble();
        d.prodDate = q.value(8).toDate();
        d.expDate = q.value(9).toDate();
        d.writtenOff = q.value(10).toBool();
        d.updateDate = q.value(11).toDate();
        d.employeeId = q.value(12).toLongLong();
        d.isDeleted = q.value(13).toBool();
        list.append(d);
    }
    return list;
}

BatchDetail StockManager::getBatch(qint64 batchId)
{
    BatchDetail d;
    if (!m_dbConnection || !m_dbConnection->isConnected() || batchId <= 0)
        return d;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT b.id, b.goodid, g.name, COALESCE(c.name,''), COALESCE(b.batchnumber,''), b.qnt, "
        "COALESCE(b.reservedquantity, 0), b.price, b.proddate, b.expdate, "
        "COALESCE(b.writtenoff, false), b.updatedate, b.emoloyeeid, COALESCE(b.isdeleted, false) "
        "FROM batches b JOIN goods g ON g.id = b.goodid "
        "LEFT JOIN goodcats c ON c.id = g.categoryid WHERE b.id = :id"));
    q.bindValue(QStringLiteral(":id"), batchId);
    if (!q.exec() || !q.next())
        return d;
    d.id = q.value(0).toLongLong();
    d.goodId = q.value(1).toLongLong();
    d.goodName = q.value(2).toString();
    d.categoryName = q.value(3).toString();
    d.batchNumber = q.value(4).toString();
    d.qnt = q.value(5).toLongLong();
    d.reservedQuantity = q.value(6).toLongLong();
    d.price = q.value(7).toDouble();
    d.prodDate = q.value(8).toDate();
    d.expDate = q.value(9).toDate();
    d.writtenOff = q.value(10).toBool();
    d.updateDate = q.value(11).toDate();
    d.employeeId = q.value(12).toLongLong();
    d.isDeleted = q.value(13).toBool();
    return d;
}

StockOperationResult StockManager::updateBatch(qint64 batchId, const QString &batchNumber, qint64 qnt,
                                              qint64 reservedQuantity, double price,
                                              const QDate &prodDate, const QDate &expDate, bool writtenOff)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (batchId <= 0) {
        r.message = QStringLiteral("Неверный ID партии");
        return r;
    }
    if (reservedQuantity > qnt) {
        r.message = QStringLiteral("Резерв не может превышать количество");
        return r;
    }
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "UPDATE batches SET batchnumber = :bn, qnt = :qnt, reservedquantity = :res, price = :price, "
        "proddate = :pdate, expdate = :edate, writtenoff = :wo, updatedate = :udate WHERE id = :id"));
    q.bindValue(QStringLiteral(":bn"), batchNumber);
    q.bindValue(QStringLiteral(":qnt"), qnt);
    q.bindValue(QStringLiteral(":res"), reservedQuantity);
    q.bindValue(QStringLiteral(":price"), price);
    q.bindValue(QStringLiteral(":pdate"), prodDate);
    q.bindValue(QStringLiteral(":edate"), expDate);
    q.bindValue(QStringLiteral(":wo"), writtenOff);
    q.bindValue(QStringLiteral(":udate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":id"), batchId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка обновления партии: %1").arg(m_lastError.text());
        return r;
    }
    r.success = true;
    r.stockId = static_cast<int>(batchId);
    r.message = QStringLiteral("Партия обновлена");
    return r;
}

StockOperationResult StockManager::createBatch(qint64 goodId, const QString &batchNumber, qint64 qnt,
                                             double price, const QDate &prodDate, const QDate &expDate,
                                             qint64 employeeId)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (!goodExists(static_cast<int>(goodId))) {
        r.message = QStringLiteral("Товар не найден");
        return r;
    }
    if (qnt <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO batches (goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price, reservedquantity) "
        "VALUES (:goodid, :qnt, :bn, :pdate, :edate, false, :udate, :empid, false, :price, 0) RETURNING id"));
    q.bindValue(QStringLiteral(":goodid"), goodId);
    q.bindValue(QStringLiteral(":qnt"), qnt);
    q.bindValue(QStringLiteral(":bn"), batchNumber.trimmed().isEmpty() ? QStringLiteral("BATCH-%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"))) : batchNumber);
    q.bindValue(QStringLiteral(":pdate"), prodDate);
    q.bindValue(QStringLiteral(":edate"), expDate);
    q.bindValue(QStringLiteral(":udate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":empid"), employeeId);
    q.bindValue(QStringLiteral(":price"), price);
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка создания партии: %1").arg(m_lastError.text());
        return r;
    }
    r.success = true;
    r.stockId = static_cast<int>(q.value(0).toLongLong());
    r.message = QStringLiteral("Партия создана");
    return r;
}

StockOperationResult StockManager::setBatchDeleted(qint64 batchId, bool deleted)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE batches SET isdeleted = :del, updatedate = :udate WHERE id = :id"));
    q.bindValue(QStringLiteral(":del"), deleted);
    q.bindValue(QStringLiteral(":udate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":id"), batchId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка: %1").arg(m_lastError.text());
        return r;
    }
    r.success = true;
    r.stockId = static_cast<int>(batchId);
    return r;
}

QList<BarcodeEntry> StockManager::getBarcodesForBatch(qint64 batchId, bool includeDeleted)
{
    QList<BarcodeEntry> list;
    if (!m_dbConnection || !m_dbConnection->isConnected() || batchId <= 0)
        return list;
    QString sql = QStringLiteral("SELECT id, barcode, batchid, updatedate, COALESCE(isdeleted, false) FROM barcodes WHERE batchid = :bid ");
    if (!includeDeleted)
        sql += QStringLiteral("AND (isdeleted IS NULL OR isdeleted = false) ");
    sql += QStringLiteral("ORDER BY barcode");
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":bid"), batchId);
    if (!q.exec())
        return list;
    while (q.next()) {
        BarcodeEntry e;
        e.id = q.value(0).toLongLong();
        e.barcode = q.value(1).toString();
        e.batchId = q.value(2).toLongLong();
        e.updateDate = q.value(3).toDate();
        e.isDeleted = q.value(4).toBool();
        list.append(e);
    }
    return list;
}

qint64 StockManager::getBatchIdByBarcode(const QString &barcode)
{
    const QString bc = barcode.trimmed();
    if (bc.isEmpty() || !m_dbConnection || !m_dbConnection->isConnected())
        return 0;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT batchid FROM barcodes WHERE barcode = :bc AND (isdeleted IS NULL OR isdeleted = false) LIMIT 1"));
    q.bindValue(QStringLiteral(":bc"), bc);
    if (!q.exec() || !q.next())
        return 0;
    return q.value(0).toLongLong();
}

StockOperationResult StockManager::addBarcodeToBatch(qint64 batchId, const QString &barcode)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    const QString bc = barcode.trimmed();
    if (bc.isEmpty()) {
        r.message = QStringLiteral("Штрихкод не задан");
        return r;
    }
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("INSERT INTO barcodes (barcode, batchid, updatedate, isdeleted) VALUES (:bc, :bid, :udate, false)"));
    q.bindValue(QStringLiteral(":bc"), bc);
    q.bindValue(QStringLiteral(":bid"), batchId);
    q.bindValue(QStringLiteral(":udate"), QDate::currentDate());
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка добавления штрихкода: %1").arg(m_lastError.text());
        return r;
    }
    r.success = true;
    return r;
}

StockOperationResult StockManager::removeBarcode(qint64 barcodeId)
{
    StockOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE barcodes SET isdeleted = true, updatedate = :udate WHERE id = :id"));
    q.bindValue(QStringLiteral(":udate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":id"), barcodeId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка удаления штрихкода: %1").arg(m_lastError.text());
        return r;
    }
    r.success = true;
    return r;
}

QSqlError StockManager::lastError() const
{
    return m_lastError;
}
