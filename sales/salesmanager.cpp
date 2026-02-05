#include "salesmanager.h"
#include "stockmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QVariant>
#include <QSet>
#include <QDebug>

SalesManager::SalesManager(QObject *parent)
    : QObject(parent)
    , m_dbConnection(nullptr)
    , m_stockManager(nullptr)
{
}

SalesManager::~SalesManager()
{
}

void SalesManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    m_dbConnection = dbConnection;
}

void SalesManager::setStockManager(StockManager *stockManager)
{
    m_stockManager = stockManager;
}

static bool dbOk(DatabaseConnection *c, SaleOperationResult &r, QSqlError &lastErr)
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

qint64 SalesManager::ensureDefaultVatRate()
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return 0;

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    if (!q.exec(QStringLiteral("SELECT id FROM vatrates WHERE (isdeleted IS NULL OR isdeleted = false) ORDER BY id LIMIT 1"))) {
        m_lastError = q.lastError();
        return 0;
    }
    if (q.next())
        return q.value(0).toLongLong();

    q.prepare(QStringLiteral("INSERT INTO vatrates (name, rate, isdeleted) VALUES ('Без НДС', 0, false) RETURNING id"));
    if (!q.exec()) {
        m_lastError = q.lastError();
        return 0;
    }
    if (q.next())
        return q.value(0).toLongLong();
    return 0;
}

qint64 SalesManager::resolveVatRateId(qint64 vatRateId)
{
    if (vatRateId > 0)
        return vatRateId;
    return ensureDefaultVatRate();
}

qint64 SalesManager::getOrCreateItemForBatch(qint64 batchId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return 0;

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id FROM items WHERE batchid = :bid AND (isdeleted IS NULL OR isdeleted = false) LIMIT 1"));
    q.bindValue(QStringLiteral(":bid"), batchId);
    if (q.exec() && q.next())
        return q.value(0).toLongLong();

    q.prepare(QStringLiteral("INSERT INTO items (batchid, serviceid, isdeleted) VALUES (:bid, NULL, false) RETURNING id"));
    q.bindValue(QStringLiteral(":bid"), batchId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        return 0;
    }
    if (q.next())
        return q.value(0).toLongLong();
    return 0;
}

qint64 SalesManager::getOrCreateItemForService(qint64 serviceId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return 0;

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id FROM items WHERE serviceid = :sid AND (isdeleted IS NULL OR isdeleted = false) LIMIT 1"));
    q.bindValue(QStringLiteral(":sid"), serviceId);
    if (q.exec() && q.next())
        return q.value(0).toLongLong();

    q.prepare(QStringLiteral("INSERT INTO items (batchid, serviceid, isdeleted) VALUES (NULL, :sid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":sid"), serviceId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        return 0;
    }
    if (q.next())
        return q.value(0).toLongLong();
    return 0;
}

double SalesManager::getBatchPrice(qint64 batchId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return -1.0;

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT price FROM batches WHERE id = :id AND (isdeleted IS NULL OR isdeleted = false)"));
    q.bindValue(QStringLiteral(":id"), batchId);
    if (q.exec() && q.next())
        return q.value(0).toDouble();
    return -1.0;
}

double SalesManager::getServicePrice(qint64 serviceId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return -1.0;

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT price FROM services WHERE id = :id AND (isdeleted IS NULL OR isdeleted = false)"));
    q.bindValue(QStringLiteral(":id"), serviceId);
    if (q.exec() && q.next())
        return q.value(0).toDouble();
    return -1.0;
}

void SalesManager::recalcCheckTotal(qint64 checkId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return;

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT COALESCE(SUM(s.sum), 0) FROM sales s WHERE s.checkid = :cid AND (s.isdeleted IS NULL OR s.isdeleted = false)"));
    q.bindValue(QStringLiteral(":cid"), checkId);
    if (!q.exec() || !q.next())
        return;

    double total = q.value(0).toDouble();

    q.prepare(QStringLiteral("UPDATE checks SET totalamount = :tot WHERE id = :id"));
    q.bindValue(QStringLiteral(":tot"), total);
    q.bindValue(QStringLiteral(":id"), checkId);
    q.exec();
}

SaleOperationResult SalesManager::createCheck(qint64 employeeId, qint64 shiftId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;

    QDate d = QDate::currentDate();
    QTime t = QTime::currentTime();

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO checks (date, \"time\", totalamount, discountamount, employeeid, shiftid, isdeleted) "
        "VALUES (:date, :time, 0, 0, :eid, :shiftid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":date"), d);
    q.bindValue(QStringLiteral(":time"), t);
    q.bindValue(QStringLiteral(":eid"), employeeId);
    q.bindValue(QStringLiteral(":shiftid"), shiftId > 0 ? shiftId : QVariant());

    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка создания чека: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }

    if (!q.next()) {
        r.message = QStringLiteral("Ошибка создания чека: не получен id");
        return r;
    }

    qint64 id = q.value(0).toLongLong();
    r.success = true;
    r.checkId = id;
    r.message = QStringLiteral("Чек создан");
    emit checkCreated(id, employeeId);
    return r;
}

Check SalesManager::getCheck(qint64 checkId, bool includeDeleted)
{
    Check c;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return c;

    QString sql = QStringLiteral(
        "SELECT ch.id, ch.date, ch.\"time\", ch.totalamount, ch.discountamount, ch.employeeid, ch.shiftid, ch.isdeleted, "
        "e.lastname || ' ' || e.firstname AS empname "
        "FROM checks ch "
        "LEFT JOIN employees e ON e.id = ch.employeeid "
        "WHERE ch.id = :id");
    if (!includeDeleted)
        sql += QStringLiteral(" AND (ch.isdeleted IS NULL OR ch.isdeleted = false)");

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":id"), checkId);
    if (!q.exec() || !q.next())
        return c;

    c.id = q.value(QStringLiteral("id")).toLongLong();
    c.date = q.value(QStringLiteral("date")).toDate();
    c.time = q.value(QStringLiteral("time")).toTime();
    c.totalAmount = q.value(QStringLiteral("totalamount")).toDouble();
    c.discountAmount = q.value(QStringLiteral("discountamount")).toDouble();
    c.employeeId = q.value(QStringLiteral("employeeid")).toLongLong();
    c.shiftId = q.value(QStringLiteral("shiftid")).toLongLong();
    c.employeeName = q.value(QStringLiteral("empname")).toString().trimmed();
    c.isDeleted = q.value(QStringLiteral("isdeleted")).toBool();
    return c;
}

QList<Check> SalesManager::getChecks(const QDate &dateFrom, const QDate &dateTo,
                                     qint64 employeeId, bool includeDeleted)
{
    QList<Check> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;

    QString sql = QStringLiteral(
        "SELECT ch.id, ch.date, ch.\"time\", ch.totalamount, ch.discountamount, ch.employeeid, ch.shiftid, ch.isdeleted, "
        "e.lastname || ' ' || e.firstname AS empname "
        "FROM checks ch "
        "LEFT JOIN employees e ON e.id = ch.employeeid "
        "WHERE ch.date BETWEEN :df AND :dt");
    if (!includeDeleted)
        sql += QStringLiteral(" AND (ch.isdeleted IS NULL OR ch.isdeleted = false)");
    if (employeeId > 0)
        sql += QStringLiteral(" AND ch.employeeid = :eid");
    sql += QStringLiteral(" ORDER BY ch.date DESC, ch.\"time\" DESC");

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":df"), dateFrom);
    q.bindValue(QStringLiteral(":dt"), dateTo);
    if (employeeId > 0)
        q.bindValue(QStringLiteral(":eid"), employeeId);

    if (!q.exec())
        return list;

    while (q.next()) {
        Check c;
        c.id = q.value(QStringLiteral("id")).toLongLong();
        c.date = q.value(QStringLiteral("date")).toDate();
        c.time = q.value(QStringLiteral("time")).toTime();
        c.totalAmount = q.value(QStringLiteral("totalamount")).toDouble();
        c.discountAmount = q.value(QStringLiteral("discountamount")).toDouble();
        c.employeeId = q.value(QStringLiteral("employeeid")).toLongLong();
        c.shiftId = q.value(QStringLiteral("shiftid")).toLongLong();
        c.employeeName = q.value(QStringLiteral("empname")).toString().trimmed();
        c.isDeleted = q.value(QStringLiteral("isdeleted")).toBool();
        list.append(c);
    }
    return list;
}

SaleOperationResult SalesManager::setCheckDiscount(qint64 checkId, double discountAmount)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (discountAmount < 0) {
        r.message = QStringLiteral("Сумма скидки не может быть отрицательной");
        return r;
    }

    Check ch = getCheck(checkId);
    if (ch.id == 0) {
        r.message = QStringLiteral("Чек не найден");
        return r;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE checks SET discountamount = :da WHERE id = :id"));
    q.bindValue(QStringLiteral(":da"), discountAmount);
    q.bindValue(QStringLiteral(":id"), checkId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка установки скидки: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }

    r.success = true;
    r.checkId = checkId;
    r.message = QStringLiteral("Скидка установлена");
    return r;
}

SaleOperationResult SalesManager::cancelCheck(qint64 checkId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;

    Check ch = getCheck(checkId);
    if (ch.id == 0) {
        r.message = QStringLiteral("Чек не найден");
        return r;
    }

    QList<SaleRow> rows = getSalesByCheck(checkId, false);
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);

    for (const SaleRow &sale : rows) {
        if (sale.batchId > 0) {
            // Получаем goodId из партии для снятия резерва
            if (m_stockManager) {
                q.prepare(QStringLiteral("SELECT goodid FROM batches WHERE id = :id"));
                q.bindValue(QStringLiteral(":id"), sale.batchId);
                if (q.exec() && q.next()) {
                    qint64 goodId = q.value(0).toLongLong();
                    // Снимаем резерв через StockManager (товар не был списан, только зарезервирован)
                    StockOperationResult releaseResult = m_stockManager->releaseReserve(
                        static_cast<int>(goodId), static_cast<double>(sale.qnt));
                    if (!releaseResult.success) {
                        // Логируем ошибку, но продолжаем операцию
                        qDebug() << "Ошибка снятия резерва при отмене чека:" << releaseResult.message;
                    }
                }
            }
            // Примечание: товар не был списан из batches при добавлении, поэтому не нужно возвращать
        }
        q.prepare(QStringLiteral("UPDATE sales SET isdeleted = true WHERE id = :id"));
        q.bindValue(QStringLiteral(":id"), sale.id);
        q.exec();
    }

    q.prepare(QStringLiteral("UPDATE checks SET isdeleted = true WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), checkId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка отмены чека: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }

    r.success = true;
    r.checkId = checkId;
    r.message = QStringLiteral("Чек отменён");
    emit checkCancelled(checkId);
    return r;
}

SaleOperationResult SalesManager::finalizeCheck(qint64 checkId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;

    Check ch = getCheck(checkId);
    if (ch.id == 0) {
        r.message = QStringLiteral("Чек не найден");
        return r;
    }

    QList<SaleRow> rows = getSalesByCheck(checkId, false);
    if (rows.isEmpty()) {
        r.message = QStringLiteral("Чек пуст");
        return r;
    }

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);

    // Списываем товар через StockManager для всех позиций с batchId
    if (m_stockManager) {
        for (const SaleRow &sale : rows) {
            if (sale.batchId > 0) {
                q.prepare(QStringLiteral("SELECT goodid FROM batches WHERE id = :id"));
                q.bindValue(QStringLiteral(":id"), sale.batchId);
                if (q.exec() && q.next()) {
                    qint64 goodId = q.value(0).toLongLong();
                    // Списываем товар через StockManager (автоматически снимет резерв)
                    StockOperationResult removeResult = m_stockManager->removeStock(
                        static_cast<int>(goodId), static_cast<double>(sale.qnt));
                    if (!removeResult.success) {
                        m_lastError = removeResult.error;
                        r.error = removeResult.error;
                        r.message = QStringLiteral("Ошибка списания товара при финализации чека: ") + removeResult.message;
                        emit operationFailed(r.message);
                        return r;
                    }
                }
            }
        }
    }

    // Помечаем чек как оплаченный (если есть поле ispaid) или просто завершаем операцию
    // В текущей схеме БД может не быть поля ispaid, поэтому просто завершаем операцию
    // Товар уже списан через StockManager

    r.success = true;
    r.checkId = checkId;
    r.message = QStringLiteral("Чек финализирован");
    emit checkFinalized(checkId, ch.totalAmount - ch.discountAmount);
    return r;
}

SaleOperationResult SalesManager::addSaleByBatchId(qint64 checkId, qint64 batchId, qint64 qnt,
                                                   qint64 vatRateId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (qnt <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }

    qint64 itemId = getOrCreateItemForBatch(batchId);
    if (itemId == 0) {
        r.message = QStringLiteral("Не удалось найти или создать номенклатуру для партии");
        r.error = m_lastError;
        return r;
    }

    double price = getBatchPrice(batchId);
    if (price < 0.0) {
        r.message = QStringLiteral("Партия не найдена или удалена");
        return r;
    }

    // Получаем goodId из партии для проверки через StockManager
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT goodid, qnt FROM batches WHERE id = :id AND (isdeleted IS NULL OR isdeleted = false)"));
    q.bindValue(QStringLiteral(":id"), batchId);
    if (!q.exec() || !q.next()) {
        r.message = QStringLiteral("Партия не найдена");
        return r;
    }
    qint64 goodId = q.value(0).toLongLong();
    qint64 batchAvailable = q.value(1).toLongLong();
    
    // Проверка через StockManager (если доступен)
    if (m_stockManager) {
        if (!m_stockManager->hasEnoughStock(static_cast<int>(goodId), static_cast<double>(qnt))) {
            Stock stock = m_stockManager->getStockByGoodId(static_cast<int>(goodId));
            r.message = QStringLiteral("Недостаточно товара на складе. Доступно: %1, требуется: %2")
                            .arg(stock.availableQuantity(), 0, 'f', 0)
                            .arg(qnt);
            return r;
        }
    } else {
        // Fallback: проверка только по партии
        if (batchAvailable < qnt) {
            r.message = QStringLiteral("Недостаточно товара в партии. Доступно: %1, требуется: %2")
                            .arg(batchAvailable).arg(qnt);
            return r;
        }
    }

    qint64 vid = resolveVatRateId(vatRateId);
    if (vid == 0) {
        r.message = QStringLiteral("Нет доступной ставки НДС");
        return r;
    }

    Check ch = getCheck(checkId);
    if (ch.id == 0) {
        r.message = QStringLiteral("Чек не найден");
        return r;
    }

    double sum = price * static_cast<double>(qnt);

    q.prepare(QStringLiteral(
        "INSERT INTO sales (itemid, qnt, vatrateid, sum, comment, checkid, isdeleted) "
        "VALUES (:iid, :qnt, :vid, :sum, NULL, :cid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":iid"), itemId);
    q.bindValue(QStringLiteral(":qnt"), qnt);
    q.bindValue(QStringLiteral(":vid"), vid);
    q.bindValue(QStringLiteral(":sum"), sum);
    q.bindValue(QStringLiteral(":cid"), checkId);

    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка добавления позиции: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }
    if (!q.next()) {
        r.message = QStringLiteral("Ошибка добавления позиции: не получен id");
        return r;
    }

    qint64 saleId = q.value(0).toLongLong();

    // Резервируем товар через StockManager (не списываем сразу - списание будет при финализации)
    if (m_stockManager) {
        StockOperationResult reserveResult = m_stockManager->reserveStock(
            static_cast<int>(goodId), static_cast<double>(qnt));
        if (!reserveResult.success) {
            m_lastError = reserveResult.error;
            r.error = reserveResult.error;
            r.message = QStringLiteral("Ошибка резервирования товара: ") + reserveResult.message;
            emit operationFailed(r.message);
            // Откатываем создание позиции
            q.prepare(QStringLiteral("DELETE FROM sales WHERE id = :id"));
            q.bindValue(QStringLiteral(":id"), saleId);
            q.exec();
            return r;
        }
    }
    // Примечание: списание из batches будет выполнено при финализации чека через StockManager::removeStock

    recalcCheckTotal(checkId);

    r.success = true;
    r.checkId = checkId;
    r.saleId = saleId;
    r.message = QStringLiteral("Позиция добавлена");
    emit saleAdded(checkId, saleId, itemId, qnt, sum);
    return r;
}

SaleOperationResult SalesManager::addSaleByServiceId(qint64 checkId, qint64 serviceId, qint64 qnt,
                                                     qint64 vatRateId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (qnt <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }

    qint64 itemId = getOrCreateItemForService(serviceId);
    if (itemId == 0) {
        r.message = QStringLiteral("Не удалось найти или создать номенклатуру для услуги");
        r.error = m_lastError;
        return r;
    }

    double price = getServicePrice(serviceId);
    if (price < 0.0) {
        r.message = QStringLiteral("Услуга не найдена или удалена");
        return r;
    }

    qint64 vid = resolveVatRateId(vatRateId);
    if (vid == 0) {
        r.message = QStringLiteral("Нет доступной ставки НДС");
        return r;
    }

    Check ch = getCheck(checkId);
    if (ch.id == 0) {
        r.message = QStringLiteral("Чек не найден");
        return r;
    }

    double sum = price * static_cast<double>(qnt);

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO sales (itemid, qnt, vatrateid, sum, comment, checkid, isdeleted) "
        "VALUES (:iid, :qnt, :vid, :sum, NULL, :cid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":iid"), itemId);
    q.bindValue(QStringLiteral(":qnt"), qnt);
    q.bindValue(QStringLiteral(":vid"), vid);
    q.bindValue(QStringLiteral(":sum"), sum);
    q.bindValue(QStringLiteral(":cid"), checkId);

    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка добавления позиции: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }
    if (!q.next()) {
        r.message = QStringLiteral("Ошибка добавления позиции: не получен id");
        return r;
    }

    qint64 saleId = q.value(0).toLongLong();
    recalcCheckTotal(checkId);

    r.success = true;
    r.checkId = checkId;
    r.saleId = saleId;
    r.message = QStringLiteral("Позиция добавлена");
    emit saleAdded(checkId, saleId, itemId, qnt, sum);
    return r;
}

SaleOperationResult SalesManager::addSale(qint64 checkId, qint64 itemId, qint64 qnt,
                                          qint64 vatRateId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (qnt <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT i.batchid, i.serviceid FROM items i "
        "WHERE i.id = :iid AND (i.isdeleted IS NULL OR i.isdeleted = false)"));
    q.bindValue(QStringLiteral(":iid"), itemId);
    if (!q.exec() || !q.next()) {
        r.message = QStringLiteral("Номенклатура не найдена");
        return r;
    }

    QVariant batchIdVar = q.value(QStringLiteral("batchid"));
    QVariant serviceIdVar = q.value(QStringLiteral("serviceid"));
    bool hasBatch = batchIdVar.isValid() && !batchIdVar.isNull();
    bool hasService = serviceIdVar.isValid() && !serviceIdVar.isNull();

    double price = -1.0;
    qint64 batchId = 0;
    qint64 goodId = 0;

    if (hasBatch) {
        batchId = batchIdVar.toLongLong();
        price = getBatchPrice(batchId);
        if (price < 0.0) {
            r.message = QStringLiteral("Партия не найдена или удалена");
            return r;
        }
        q.prepare(QStringLiteral("SELECT goodid, qnt FROM batches WHERE id = :id AND (isdeleted IS NULL OR isdeleted = false)"));
        q.bindValue(QStringLiteral(":id"), batchId);
        if (!q.exec() || !q.next()) {
            r.message = QStringLiteral("Партия не найдена");
            return r;
        }
        goodId = q.value(0).toLongLong();
        qint64 batchAvailable = q.value(1).toLongLong();
        
        // Проверка через StockManager (если доступен)
        if (m_stockManager) {
            if (!m_stockManager->hasEnoughStock(static_cast<int>(goodId), static_cast<double>(qnt))) {
                Stock stock = m_stockManager->getStockByGoodId(static_cast<int>(goodId));
                r.message = QStringLiteral("Недостаточно товара на складе. Доступно: %1, требуется: %2")
                                .arg(stock.availableQuantity(), 0, 'f', 0)
                                .arg(qnt);
                return r;
            }
        } else {
            // Fallback: проверка только по партии
            if (batchAvailable < qnt) {
                r.message = QStringLiteral("Недостаточно товара в партии. Доступно: %1, требуется: %2")
                                .arg(batchAvailable).arg(qnt);
                return r;
            }
        }
    } else if (hasService) {
        price = getServicePrice(serviceIdVar.toLongLong());
        if (price < 0.0) {
            r.message = QStringLiteral("Услуга не найдена или удалена");
            return r;
        }
    } else {
        r.message = QStringLiteral("Номенклатура не привязана ни к партии, ни к услуге");
        return r;
    }

    qint64 vid = resolveVatRateId(vatRateId);
    if (vid == 0) {
        r.message = QStringLiteral("Нет доступной ставки НДС");
        return r;
    }

    Check ch = getCheck(checkId);
    if (ch.id == 0) {
        r.message = QStringLiteral("Чек не найден");
        return r;
    }

    double sum = price * static_cast<double>(qnt);

    q.prepare(QStringLiteral(
        "INSERT INTO sales (itemid, qnt, vatrateid, sum, comment, checkid, isdeleted) "
        "VALUES (:iid, :qnt, :vid, :sum, NULL, :cid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":iid"), itemId);
    q.bindValue(QStringLiteral(":qnt"), qnt);
    q.bindValue(QStringLiteral(":vid"), vid);
    q.bindValue(QStringLiteral(":sum"), sum);
    q.bindValue(QStringLiteral(":cid"), checkId);

    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка добавления позиции: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }
    if (!q.next()) {
        r.message = QStringLiteral("Ошибка добавления позиции: не получен id");
        return r;
    }

    qint64 saleId = q.value(0).toLongLong();

    if (batchId > 0) {
        // Резервируем товар через StockManager (не списываем сразу - списание будет при финализации)
        if (m_stockManager) {
            StockOperationResult reserveResult = m_stockManager->reserveStock(
                static_cast<int>(goodId), static_cast<double>(qnt));
            if (!reserveResult.success) {
                m_lastError = reserveResult.error;
                r.error = reserveResult.error;
                r.message = QStringLiteral("Ошибка резервирования товара: ") + reserveResult.message;
                emit operationFailed(r.message);
                // Откатываем создание позиции
                q.prepare(QStringLiteral("DELETE FROM sales WHERE id = :id"));
                q.bindValue(QStringLiteral(":id"), saleId);
                q.exec();
                return r;
            }
        }
        // Примечание: списание из batches будет выполнено при финализации чека через StockManager::removeStock
    }

    recalcCheckTotal(checkId);

    r.success = true;
    r.checkId = checkId;
    r.saleId = saleId;
    r.message = QStringLiteral("Позиция добавлена");
    emit saleAdded(checkId, saleId, itemId, qnt, sum);
    return r;
}

SaleOperationResult SalesManager::removeSale(qint64 saleId)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;

    SaleRow row = getSale(saleId, false);
    if (row.id == 0) {
        r.message = QStringLiteral("Позиция продажи не найдена");
        return r;
    }

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);

    if (row.batchId > 0) {
        // Получаем goodId из партии для снятия резерва
        if (m_stockManager) {
            q.prepare(QStringLiteral("SELECT goodid FROM batches WHERE id = :id"));
            q.bindValue(QStringLiteral(":id"), row.batchId);
            if (q.exec() && q.next()) {
                qint64 goodId = q.value(0).toLongLong();
                // Снимаем резерв через StockManager (товар не был списан, только зарезервирован)
                StockOperationResult releaseResult = m_stockManager->releaseReserve(
                    static_cast<int>(goodId), static_cast<double>(row.qnt));
                if (!releaseResult.success) {
                    // Логируем ошибку, но продолжаем операцию
                    qDebug() << "Ошибка снятия резерва при удалении позиции:" << releaseResult.message;
                }
            }
        }
        // Примечание: товар не был списан из batches при добавлении, поэтому не нужно возвращать
    }

    q.prepare(QStringLiteral("UPDATE sales SET isdeleted = true WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка удаления позиции: ") + m_lastError.text();
        emit operationFailed(r.message);
        return r;
    }

    recalcCheckTotal(row.checkId);

    r.success = true;
    r.checkId = row.checkId;
    r.saleId = saleId;
    r.message = QStringLiteral("Позиция удалена");
    emit saleRemoved(row.checkId, saleId);
    return r;
}

SaleOperationResult SalesManager::updateSaleQuantity(qint64 saleId, qint64 newQnt)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (newQnt <= 0) {
        r.message = QStringLiteral("Количество должно быть больше нуля");
        return r;
    }

    SaleRow row = getSale(saleId, false);
    if (row.id == 0) {
        r.message = QStringLiteral("Позиция не найдена");
        return r;
    }

    if (row.qnt == newQnt) {
        r.success = true;
        r.checkId = row.checkId;
        r.saleId = saleId;
        return r;
    }

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);

    int goodIdForStock = 0;
    if (row.batchId > 0) {
        q.prepare(QStringLiteral("SELECT goodid, qnt FROM batches WHERE id = :id"));
        q.bindValue(QStringLiteral(":id"), row.batchId);
        if (!q.exec() || !q.next()) {
            r.message = QStringLiteral("Партия не найдена");
            return r;
        }
        goodIdForStock = static_cast<int>(q.value(QStringLiteral("goodid")).toLongLong());
        double batchAvailable = q.value(QStringLiteral("qnt")).toDouble();
        if (m_stockManager) {
            QList<Stock> stocks = m_stockManager->getStocksByGoodId(goodIdForStock, false);
            batchAvailable = 0;
            for (const Stock &s : stocks)
                batchAvailable += s.availableQuantity();
        }
        double needMore = static_cast<double>(newQnt) - static_cast<double>(row.qnt);
        if (needMore > 0 && batchAvailable < needMore) {
            r.message = QStringLiteral("Недостаточно товара. Доступно: %1").arg(QString::number(batchAvailable, 'f', 2));
            return r;
        }
        if (m_stockManager) {
            StockOperationResult rel = m_stockManager->releaseReserve(goodIdForStock, static_cast<double>(row.qnt));
            if (!rel.success) {
                r.message = rel.message;
                return r;
            }
            StockOperationResult res = m_stockManager->reserveStock(goodIdForStock, static_cast<double>(newQnt));
            if (!res.success) {
                m_stockManager->reserveStock(goodIdForStock, static_cast<double>(row.qnt));
                r.message = res.message;
                return r;
            }
        }
    }

    const double newSum = row.unitPrice * static_cast<double>(newQnt);
    q.prepare(QStringLiteral("UPDATE sales SET qnt = :qnt, sum = :sum WHERE id = :id"));
    q.bindValue(QStringLiteral(":qnt"), newQnt);
    q.bindValue(QStringLiteral(":sum"), newSum);
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка изменения количества: ") + m_lastError.text();
        if (row.batchId > 0 && m_stockManager && goodIdForStock > 0) {
            m_stockManager->releaseReserve(goodIdForStock, static_cast<double>(newQnt));
            m_stockManager->reserveStock(goodIdForStock, static_cast<double>(row.qnt));
        }
        return r;
    }

    recalcCheckTotal(row.checkId);

    r.success = true;
    r.checkId = row.checkId;
    r.saleId = saleId;
    r.message = QStringLiteral("Количество изменено");
    return r;
}

QList<SaleRow> SalesManager::getSalesByCheck(qint64 checkId, bool includeDeleted)
{
    QList<SaleRow> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;

    QString sql = QStringLiteral(
        "SELECT s.id, s.itemid, s.qnt, s.vatrateid, s.sum, s.comment, s.checkid, s.isdeleted, "
        "i.batchid, i.serviceid, "
        "COALESCE(g.name, sv.name) AS itemname, "
        "CASE WHEN s.qnt > 0 THEN s.sum / s.qnt ELSE 0 END AS unitprice "
        "FROM sales s "
        "JOIN items i ON i.id = s.itemid "
        "LEFT JOIN batches b ON b.id = i.batchid "
        "LEFT JOIN goods g ON g.id = b.goodid "
        "LEFT JOIN services sv ON sv.id = i.serviceid "
        "WHERE s.checkid = :cid");
    if (!includeDeleted)
        sql += QStringLiteral(" AND (s.isdeleted IS NULL OR s.isdeleted = false)");
    sql += QStringLiteral(" ORDER BY s.id");

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":cid"), checkId);
    if (!q.exec())
        return list;

    while (q.next()) {
        SaleRow row;
        row.id = q.value(QStringLiteral("id")).toLongLong();
        row.itemId = q.value(QStringLiteral("itemid")).toLongLong();
        row.qnt = q.value(QStringLiteral("qnt")).toLongLong();
        row.vatRateId = q.value(QStringLiteral("vatrateid")).toLongLong();
        row.sum = q.value(QStringLiteral("sum")).toDouble();
        row.comment = q.value(QStringLiteral("comment")).toString();
        row.checkId = q.value(QStringLiteral("checkid")).toLongLong();
        row.isDeleted = q.value(QStringLiteral("isdeleted")).toBool();
        row.batchId = q.value(QStringLiteral("batchid")).toLongLong();
        row.serviceId = q.value(QStringLiteral("serviceid")).toLongLong();
        row.itemName = q.value(QStringLiteral("itemname")).toString();
        row.unitPrice = q.value(QStringLiteral("unitprice")).toDouble();
        list.append(row);
    }
    return list;
}

SaleRow SalesManager::getSale(qint64 saleId, bool includeDeleted)
{
    SaleRow row;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return row;

    QString sql = QStringLiteral(
        "SELECT s.id, s.itemid, s.qnt, s.vatrateid, s.sum, s.comment, s.checkid, s.isdeleted, "
        "i.batchid, i.serviceid, COALESCE(g.name, sv.name) AS itemname "
        "FROM sales s "
        "JOIN items i ON i.id = s.itemid "
        "LEFT JOIN batches b ON b.id = i.batchid "
        "LEFT JOIN goods g ON g.id = b.goodid "
        "LEFT JOIN services sv ON sv.id = i.serviceid "
        "WHERE s.id = :id");
    if (!includeDeleted)
        sql += QStringLiteral(" AND (s.isdeleted IS NULL OR s.isdeleted = false)");

    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec() || !q.next())
        return row;

    row.id = q.value(QStringLiteral("id")).toLongLong();
    row.itemId = q.value(QStringLiteral("itemid")).toLongLong();
    row.qnt = q.value(QStringLiteral("qnt")).toLongLong();
    row.vatRateId = q.value(QStringLiteral("vatrateid")).toLongLong();
    row.sum = q.value(QStringLiteral("sum")).toDouble();
    row.comment = q.value(QStringLiteral("comment")).toString();
    row.checkId = q.value(QStringLiteral("checkid")).toLongLong();
    row.isDeleted = q.value(QStringLiteral("isdeleted")).toBool();
    row.batchId = q.value(QStringLiteral("batchid")).toLongLong();
    row.serviceId = q.value(QStringLiteral("serviceid")).toLongLong();
    row.itemName = q.value(QStringLiteral("itemname")).toString();
    row.unitPrice = row.qnt > 0 ? row.sum / static_cast<double>(row.qnt) : 0.0;
    return row;
}

QList<BatchInfo> SalesManager::getAvailableBatches()
{
    QList<BatchInfo> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;

    QString sql = QStringLiteral(
        "SELECT b.id AS batchid, g.id AS goodid, g.name AS goodname, "
        "COALESCE(c.name, '') AS categoryname, b.price, b.qnt, COALESCE(b.batchnumber, '') AS batchnumber, "
        "g.imageurl "
        "FROM batches b "
        "JOIN goods g ON g.id = b.goodid "
        "LEFT JOIN goodcats c ON c.id = g.categoryid "
        "WHERE b.qnt > 0 AND (b.isdeleted IS NULL OR b.isdeleted = false) "
        "AND (g.isdeleted IS NULL OR g.isdeleted = false) AND g.isactive = true "
        "ORDER BY g.name, b.id");

    QSqlQuery q(m_dbConnection->getDatabase());
    if (!q.exec(sql))
        return list;

    // Если StockManager доступен, фильтруем по доступным остаткам
    QSet<qint64> goodsWithStock;
    if (m_stockManager) {
        QList<Stock> allStocks = m_stockManager->getAllStocks(false);
        for (const Stock &stock : allStocks) {
            if (stock.availableQuantity() > 0.0) {
                goodsWithStock.insert(static_cast<qint64>(stock.goodId));
            }
        }
    }

    while (q.next()) {
        qint64 goodId = q.value(QStringLiteral("goodid")).toLongLong();
        
        // Если StockManager доступен, проверяем наличие остатка
        if (m_stockManager) {
            if (!goodsWithStock.contains(goodId)) {
                continue; // Пропускаем товары без доступного остатка
            }
        }
        
        BatchInfo info;
        info.batchId = q.value(QStringLiteral("batchid")).toLongLong();
        info.goodId = goodId;
        info.goodName = q.value(QStringLiteral("goodname")).toString();
        info.categoryName = q.value(QStringLiteral("categoryname")).toString();
        info.price = q.value(QStringLiteral("price")).toDouble();
        info.qnt = q.value(QStringLiteral("qnt")).toLongLong();
        info.batchNumber = q.value(QStringLiteral("batchnumber")).toString();
        info.imageUrl = q.value(QStringLiteral("imageurl")).toString();
        list.append(info);
    }
    return list;
}

QList<ServiceInfo> SalesManager::getAvailableServices()
{
    QList<ServiceInfo> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;

    QString sql = QStringLiteral(
        "SELECT s.id AS serviceid, s.name AS servicename, "
        "COALESCE(c.name, '') AS categoryname, s.price "
        "FROM services s "
        "LEFT JOIN goodcats c ON c.id = s.categoryid "
        "WHERE (s.isdeleted IS NULL OR s.isdeleted = false) AND s.isactive = true "
        "ORDER BY s.name");

    QSqlQuery q(m_dbConnection->getDatabase());
    if (!q.exec(sql))
        return list;

    while (q.next()) {
        ServiceInfo info;
        info.serviceId = q.value(QStringLiteral("serviceid")).toLongLong();
        info.serviceName = q.value(QStringLiteral("servicename")).toString();
        info.categoryName = q.value(QStringLiteral("categoryname")).toString();
        info.price = q.value(QStringLiteral("price")).toDouble();
        list.append(info);
    }
    return list;
}

QList<PaymentMethodInfo> SalesManager::getPaymentMethods()
{
    QList<PaymentMethodInfo> list;
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return list;
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id, name, COALESCE(sortorder, 0) FROM paymentmethods "
        "WHERE (isdeleted IS NULL OR isdeleted = false) AND isactive = true ORDER BY sortorder, name"));
    if (!q.exec())
        return list;
    while (q.next()) {
        PaymentMethodInfo info;
        info.id = q.value(0).toLongLong();
        info.name = q.value(1).toString();
        info.sortOrder = q.value(2).toInt();
        info.isActive = true;
        list.append(info);
    }
    return list;
}

SaleOperationResult SalesManager::recordCheckPayments(qint64 checkId, const QList<CheckPaymentRow> &payments)
{
    SaleOperationResult r;
    if (!dbOk(m_dbConnection, r, m_lastError))
        return r;
    if (checkId <= 0) {
        r.message = QStringLiteral("Неверный ID чека");
        return r;
    }
    QSqlQuery del(m_dbConnection->getDatabase());
    del.prepare(QStringLiteral("DELETE FROM checkpayments WHERE checkid = :cid"));
    del.bindValue(QStringLiteral(":cid"), checkId);
    if (!del.exec()) {
        m_lastError = del.lastError();
        r.error = m_lastError;
        r.message = QStringLiteral("Ошибка очистки оплат: %1").arg(m_lastError.text());
        return r;
    }
    for (const CheckPaymentRow &row : payments) {
        if (row.amount <= 0 || row.paymentMethodId <= 0)
            continue;
        QSqlQuery ins(m_dbConnection->getDatabase());
        ins.prepare(QStringLiteral(
            "INSERT INTO checkpayments (checkid, paymentmethodid, amount) VALUES (:cid, :pmid, :amt)"));
        ins.bindValue(QStringLiteral(":cid"), checkId);
        ins.bindValue(QStringLiteral(":pmid"), row.paymentMethodId);
        ins.bindValue(QStringLiteral(":amt"), row.amount);
        if (!ins.exec()) {
            m_lastError = ins.lastError();
            r.error = m_lastError;
            r.message = QStringLiteral("Ошибка записи оплаты: %1").arg(m_lastError.text());
            return r;
        }
    }
    r.success = true;
    r.checkId = checkId;
    r.message = QStringLiteral("Оплаты записаны");
    return r;
}

QSqlError SalesManager::lastError() const
{
    return m_lastError;
}
