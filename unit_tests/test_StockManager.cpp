#include "test_StockManager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QUuid>
#include <QDate>

TestStockManager::TestStockManager()
{
}

TestStockManager::~TestStockManager()
{
}

void TestStockManager::initTestCase()
{
    qDebug() << "Инициализация тестового набора для StockManager";
}

void TestStockManager::cleanupTestCase()
{
    qDebug() << "Завершение тестового набора для StockManager";
    cleanupTestData();
    cleanupDatabaseConnections();
}

void TestStockManager::init()
{
}

void TestStockManager::cleanup()
{
    cleanupTestData();
    cleanupDatabaseConnections();
}

PostgreSQLAuth TestStockManager::createTestAuth() const
{
    PostgreSQLAuth auth;
    auth.host = "192.168.0.202";
    auth.port = 5432;
    auth.database = "pos_bakery";
    auth.username = "postgres";
    auth.password = "123456";
    auth.sslMode = "prefer";
    auth.connectTimeout = 10;
    return auth;
}

DatabaseConnection* TestStockManager::createTestDatabaseConnection()
{
    DatabaseConnection* dbConn = new DatabaseConnection();
    PostgreSQLAuth auth = createTestAuth();
    
    if (dbConn->connect(auth)) {
        return dbConn;
    } else {
        delete dbConn;
        return nullptr;
    }
}

void TestStockManager::cleanupDatabaseConnections()
{
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &name : connections) {
        if (name.startsWith("easyPOS_connection_")) {
            QSqlDatabase::removeDatabase(name);
        }
    }
}

int TestStockManager::createTestGood(const QString &name)
{
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return 0;
    
    QString goodName = name.isEmpty() ? QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)) : name;
    
    QSqlQuery q(dbConn->getDatabase());
    
    int categoryId = 1;
    q.prepare(QStringLiteral("SELECT id FROM goodcats WHERE id = 1"));
    if (!q.exec() || !q.next()) {
        q.prepare(QStringLiteral("INSERT INTO goodcats (id, name, description, updatedate, isdeleted) VALUES (1, 'Test', 'Test', CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));
        q.exec();
        categoryId = 1;
    }
    
    int employeeId = 1;
    q.prepare(QStringLiteral("SELECT id FROM employees WHERE id = 1"));
    if (!q.exec() || !q.next()) {
        q.prepare(QStringLiteral("INSERT INTO positions (id, name, isactive, updatedate, isdeleted) VALUES (1, 'Test', true, CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));
        q.exec();
        q.prepare(QStringLiteral("INSERT INTO employees (id, lastname, firstname, positionid, updatedate, isdeleted) VALUES (1, 'Test', 'Test', 1, CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));
        q.exec();
        employeeId = 1;
    }
    
    q.prepare(QStringLiteral("INSERT INTO goods (name, description, categoryid, isactive, updatedate, employeeid, isdeleted) VALUES (:name, 'Test', :catid, true, CURRENT_DATE, :empid, false) RETURNING id"));
    q.bindValue(QStringLiteral(":name"), goodName);
    q.bindValue(QStringLiteral(":catid"), categoryId);
    q.bindValue(QStringLiteral(":empid"), employeeId);
    
    if (!q.exec() || !q.next())
        return 0;
    
    return q.value(0).toInt();
}

void TestStockManager::cleanupTestData()
{
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return;
    
    QSqlQuery q(dbConn->getDatabase());
    q.exec(QStringLiteral("DELETE FROM batches WHERE batchnumber LIKE 'STOCK-%' OR batchnumber LIKE 'ADD-%'"));
    q.exec(QStringLiteral("DELETE FROM goods WHERE name LIKE 'TEST_GOOD_%'"));
    
    delete dbConn;
}

void TestStockManager::testConstructor()
{
    StockManager sm;
    QVERIFY(!sm.hasEnoughStock(99999, 1.0));
}

void TestStockManager::testSetDatabaseConnection()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn) {
        sm.setDatabaseConnection(dbConn);
        QVERIFY(true);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testSetDatabaseConnectionNull()
{
    StockManager sm;
    sm.setDatabaseConnection(nullptr);
    
    StockOperationResult r = sm.setStock(1, 100.0);
    QVERIFY(!r.success);
    QVERIFY(r.message.contains("Нет подключения"));
}

void TestStockManager::testSetStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        StockOperationResult r = sm.setStock(goodId, 100.0);
        QVERIFY(r.success);
        QVERIFY(r.stockId == goodId);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(s.id == goodId);
        QVERIFY(qAbs(s.quantity - 100.0) < 0.01);
        QVERIFY(s.reservedQuantity == 0.0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testSetStockByGoodName()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        QString goodName = QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        int goodId = createTestGood(goodName);
        QVERIFY(goodId > 0);
        
        StockOperationResult r = sm.setStock(goodName, 50.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodName(goodName);
        QVERIFY(s.id == goodId);
        QVERIFY(qAbs(s.quantity - 50.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testSetStockWithoutConnection()
{
    StockManager sm;
    StockOperationResult r = sm.setStock(1, 100.0);
    QVERIFY(!r.success);
    QVERIFY(r.message.contains("Нет подключения"));
}

void TestStockManager::testSetStockInvalidGood()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        StockOperationResult r = sm.setStock(99999, 100.0);
        QVERIFY(!r.success);
        QVERIFY(r.message.contains("не найден"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testSetStockNegativeQuantity()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        StockOperationResult r = sm.setStock(goodId, -10.0);
        QVERIFY(!r.success);
        QVERIFY(r.message.contains("отрицательным"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testAddStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 50.0);
        
        StockOperationResult r = sm.addStock(goodId, 30.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.quantity - 80.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testAddStockByGoodName()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        QString goodName = QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        int goodId = createTestGood(goodName);
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 20.0);
        
        StockOperationResult r = sm.addStock(goodName, 15.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodName(goodName);
        QVERIFY(qAbs(s.quantity - 35.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testAddStockMultipleBatches()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.addStock(goodId, 10.0);
        sm.addStock(goodId, 20.0);
        sm.addStock(goodId, 30.0);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.quantity - 60.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testGetStockByGoodId()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 75.0);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(s.id == goodId);
        QVERIFY(qAbs(s.quantity - 75.0) < 0.01);
        QVERIFY(!s.goodName.isEmpty());
        
        Stock nonExistent = sm.getStockByGoodId(99999);
        QVERIFY(nonExistent.id == 0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testGetStockByGoodName()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        QString goodName = QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        int goodId = createTestGood(goodName);
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 90.0);
        
        Stock s = sm.getStockByGoodName(goodName);
        QVERIFY(s.id == goodId);
        QVERIFY(s.goodName == goodName);
        QVERIFY(qAbs(s.quantity - 90.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testGetAllStocks()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId1 = createTestGood();
        int goodId2 = createTestGood();
        QVERIFY(goodId1 > 0 && goodId2 > 0);
        
        sm.setStock(goodId1, 40.0);
        sm.setStock(goodId2, 60.0);
        
        QList<Stock> stocks = sm.getAllStocks();
        QVERIFY(stocks.size() >= 2);
        
        bool found1 = false, found2 = false;
        for (const Stock &s : stocks) {
            if (s.id == goodId1 && qAbs(s.quantity - 40.0) < 0.01)
                found1 = true;
            if (s.id == goodId2 && qAbs(s.quantity - 60.0) < 0.01)
                found2 = true;
        }
        QVERIFY(found1 && found2);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testGetStocksByGoodId()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 25.0);
        
        QList<Stock> stocks = sm.getStocksByGoodId(goodId);
        QVERIFY(stocks.size() == 1);
        QVERIFY(stocks[0].id == goodId);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testRemoveStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 100.0);
        
        StockOperationResult r = sm.removeStock(goodId, 30.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.quantity - 70.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testRemoveStockByGoodName()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        QString goodName = QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        int goodId = createTestGood(goodName);
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 80.0);
        
        StockOperationResult r = sm.removeStock(goodName, 25.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodName(goodName);
        QVERIFY(qAbs(s.quantity - 55.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testRemoveStockInsufficient()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 50.0);
        
        StockOperationResult r = sm.removeStock(goodId, 100.0);
        QVERIFY(!r.success);
        QVERIFY(r.message.contains("Недостаточно"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testRemoveStockFIFO()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.addStock(goodId, 20.0);
        sm.addStock(goodId, 30.0);
        
        StockOperationResult r = sm.removeStock(goodId, 25.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.quantity - 25.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testReserveStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 100.0);
        
        StockOperationResult r = sm.reserveStock(goodId, 40.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.quantity - 100.0) < 0.01);
        QVERIFY(qAbs(s.reservedQuantity - 40.0) < 0.01);
        QVERIFY(qAbs(s.availableQuantity() - 60.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testReserveStockByGoodName()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        QString goodName = QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        int goodId = createTestGood(goodName);
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 80.0);
        
        StockOperationResult r = sm.reserveStock(goodName, 25.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodName(goodName);
        QVERIFY(qAbs(s.reservedQuantity - 25.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testReserveStockInsufficient()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 50.0);
        
        StockOperationResult r = sm.reserveStock(goodId, 100.0);
        QVERIFY(!r.success);
        QVERIFY(r.message.contains("Недостаточно"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testReleaseReserve()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 100.0);
        sm.reserveStock(goodId, 60.0);
        
        StockOperationResult r = sm.releaseReserve(goodId, 30.0);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.reservedQuantity - 30.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testReleaseReserveInsufficient()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 100.0);
        sm.reserveStock(goodId, 20.0);
        
        StockOperationResult r = sm.releaseReserve(goodId, 50.0);
        QVERIFY(!r.success);
        QVERIFY(r.message.contains("Недостаточно"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testHasEnoughStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 100.0);
        sm.reserveStock(goodId, 30.0);
        
        QVERIFY(sm.hasEnoughStock(goodId, 50.0));
        QVERIFY(!sm.hasEnoughStock(goodId, 100.0));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testHasEnoughStockByGoodName()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        QString goodName = QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        int goodId = createTestGood(goodName);
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 75.0);
        
        QVERIFY(sm.hasEnoughStock(goodName, 50.0));
        QVERIFY(!sm.hasEnoughStock(goodName, 100.0));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testStockExists()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        QVERIFY(!sm.stockExists(goodId));
        
        sm.setStock(goodId, 50.0);
        QVERIFY(sm.stockExists(goodId));
        
        QVERIFY(!sm.stockExists(99999));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testDeleteStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 100.0);
        QVERIFY(sm.stockExists(goodId));
        
        StockOperationResult r = sm.deleteStock(goodId);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId, true);
        QVERIFY(s.quantity == 0.0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestStockManager::testRestoreStock()
{
    StockManager sm;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        sm.setDatabaseConnection(dbConn);
        
        int goodId = createTestGood();
        QVERIFY(goodId > 0);
        
        sm.setStock(goodId, 50.0);
        sm.deleteStock(goodId);
        
        StockOperationResult r = sm.restoreStock(goodId);
        QVERIFY(r.success);
        
        Stock s = sm.getStockByGoodId(goodId);
        QVERIFY(qAbs(s.quantity - 50.0) < 0.01);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}
