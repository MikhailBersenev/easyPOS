#include "test_SalesManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QDate>

TestSalesManager::TestSalesManager()
{
}

TestSalesManager::~TestSalesManager()
{
}

void TestSalesManager::initTestCase()
{
    qDebug() << "Инициализация тестового набора для SalesManager";
}

void TestSalesManager::cleanupTestCase()
{
    qDebug() << "Завершение тестового набора для SalesManager";
    cleanupTestData();
    TestHelpers::cleanupDatabaseConnections();
}

void TestSalesManager::init()
{
}

void TestSalesManager::cleanup()
{
    cleanupTestData();
    TestHelpers::cleanupDatabaseConnections();
}

int TestSalesManager::createTestGood(const QString &name)
{
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return 0;

    QString goodName = name.isEmpty() ? QString("TEST_GOOD_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)) : name;
    QSqlQuery q(dbConn->getDatabase());

    q.exec(QStringLiteral("INSERT INTO goodcats (id, name, description, updatedate, isdeleted) VALUES (9998, 'TestCat', 'Test', CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));
    q.exec(QStringLiteral("INSERT INTO positions (id, name, isactive, updatedate, isdeleted) VALUES (9998, 'Test', true, CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));
    q.exec(QStringLiteral("INSERT INTO employees (id, lastname, firstname, positionid, updatedate, isdeleted) VALUES (9998, 'Test', 'Test', 9998, CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));

    q.prepare(QStringLiteral("INSERT INTO goods (name, description, categoryid, isactive, updatedate, employeeid, isdeleted) VALUES (:name, 'Test', 9998, true, CURRENT_DATE, 9998, false) RETURNING id"));
    q.bindValue(QStringLiteral(":name"), goodName);
    if (!q.exec() || !q.next()) {
        delete dbConn;
        return 0;
    }
    int id = q.value(0).toInt();
    delete dbConn;
    return id;
}

qint64 TestSalesManager::createTestEmployee()
{
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return 0;

    QSqlQuery q(dbConn->getDatabase());
    q.exec(QStringLiteral("INSERT INTO positions (id, name, isactive, updatedate, isdeleted) VALUES (9997, 'TestPos', true, CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));
    q.exec(QStringLiteral("INSERT INTO employees (id, lastname, firstname, positionid, updatedate, isdeleted) VALUES (9997, 'TestEmp', 'Test', 9997, CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));

    q.exec(QStringLiteral("SELECT id FROM employees WHERE id = 9997"));
    qint64 empId = 0;
    if (q.next())
        empId = q.value(0).toLongLong();
    delete dbConn;
    return empId;
}

qint64 TestSalesManager::createTestBatch(int goodId, double price, qint64 qnt)
{
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return 0;

    QSqlQuery q(dbConn->getDatabase());
    QString batchnum = QString("TEST-BATCH-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    q.prepare(QStringLiteral(
        "INSERT INTO batches (goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price, reservedquantity) "
        "VALUES (:gid, :qnt, :bn, CURRENT_DATE, CURRENT_DATE + 365, false, CURRENT_DATE, 1, false, :price, 0) RETURNING id"));
    q.bindValue(QStringLiteral(":gid"), goodId);
    q.bindValue(QStringLiteral(":qnt"), qnt);
    q.bindValue(QStringLiteral(":bn"), batchnum);
    q.bindValue(QStringLiteral(":price"), price);

    if (!q.exec() || !q.next()) {
        delete dbConn;
        return 0;
    }
    qint64 id = q.value(0).toLongLong();
    delete dbConn;
    return id;
}

qint64 TestSalesManager::createTestService(const QString &name, double price)
{
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return 0;

    QString svcName = name.isEmpty() ? QString("TEST_SVC_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)) : name;
    QSqlQuery q(dbConn->getDatabase());

    q.exec(QStringLiteral("INSERT INTO goodcats (id, name, description, updatedate, isdeleted) VALUES (9996, 'SvcCat', 'Test', CURRENT_DATE, false) ON CONFLICT (id) DO NOTHING"));

    q.prepare(QStringLiteral(
        "INSERT INTO services (name, description, price, updatedate, isactive, categoryid, isdeleted) "
        "VALUES (:name, 'Test', :price, CURRENT_DATE, true, 9996, false) RETURNING id"));
    q.bindValue(QStringLiteral(":name"), svcName);
    q.bindValue(QStringLiteral(":price"), price);

    if (!q.exec() || !q.next()) {
        delete dbConn;
        return 0;
    }
    qint64 id = q.value(0).toLongLong();
    delete dbConn;
    return id;
}

void TestSalesManager::cleanupTestData()
{
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected())
        return;

    QSqlQuery q(dbConn->getDatabase());
    q.exec(QStringLiteral("DELETE FROM sales WHERE checkid IN (SELECT id FROM checks WHERE employeeid IN (9997, 9998))"));
    q.exec(QStringLiteral("DELETE FROM checks WHERE employeeid IN (9997, 9998)"));
    q.exec(QStringLiteral("DELETE FROM items WHERE batchid IN (SELECT id FROM batches WHERE batchnumber LIKE 'TEST-BATCH-%')"));
    q.exec(QStringLiteral("DELETE FROM items WHERE serviceid IN (SELECT id FROM services WHERE name LIKE 'TEST_SVC_%')"));
    q.exec(QStringLiteral("DELETE FROM batches WHERE batchnumber LIKE 'TEST-BATCH-%'"));
    q.exec(QStringLiteral("DELETE FROM services WHERE name LIKE 'TEST_SVC_%'"));
    q.exec(QStringLiteral("DELETE FROM goods WHERE name LIKE 'TEST_GOOD_%' OR categoryid = 9998"));
    q.exec(QStringLiteral("DELETE FROM goodcats WHERE id IN (9996, 9998)"));
    q.exec(QStringLiteral("DELETE FROM employees WHERE id IN (9997, 9998)"));
    q.exec(QStringLiteral("DELETE FROM positions WHERE id IN (9997, 9998)"));

    delete dbConn;
}

void TestSalesManager::testConstructor()
{
    SalesManager sm;
    SaleOperationResult r = sm.createCheck(1);
    QVERIFY(!r.success);
}

void TestSalesManager::testSetDatabaseConnection()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (dbConn) {
        sm.setDatabaseConnection(dbConn);
        QVERIFY(true);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestSalesManager::testCreateCheck()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }

    sm.setDatabaseConnection(dbConn);
    qint64 empId = createTestEmployee();
    if (empId <= 0) {
        QSKIP("Не удалось создать тестового сотрудника");
        return;
    }

    SaleOperationResult r = sm.createCheck(empId);
    QVERIFY(r.success);
    QVERIFY(r.checkId > 0);

    Check ch = sm.getCheck(r.checkId);
    QVERIFY(ch.id == r.checkId);
    QVERIFY(ch.employeeId == empId);
    QVERIFY(qAbs(ch.totalAmount - 0.0) < 0.01);
}

void TestSalesManager::testCreateCheckWithoutConnection()
{
    SalesManager sm;
    SaleOperationResult r = sm.createCheck(1);
    QVERIFY(!r.success);
    QVERIFY(r.message.contains("подключения"));
}

void TestSalesManager::testCreateCheckInvalidEmployee()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    SaleOperationResult r = sm.createCheck(99999999);
    QVERIFY(!r.success);
}

void TestSalesManager::testGetCheck()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);
    qint64 empId = createTestEmployee();
    if (empId <= 0) {
        QSKIP("Не удалось создать тестового сотрудника");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);

    Check ch = sm.getCheck(cr.checkId);
    QVERIFY(ch.id == cr.checkId);
    QVERIFY(ch.date == QDate::currentDate());
}

void TestSalesManager::testAddSaleByBatchId()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    int goodId = createTestGood();
    qint64 empId = createTestEmployee();
    if (goodId <= 0 || empId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    qint64 batchId = createTestBatch(goodId, 100.0, 10);
    QVERIFY(batchId > 0);

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);

    SaleOperationResult ar = sm.addSaleByBatchId(cr.checkId, batchId, 2);
    QVERIFY(ar.success);
    QVERIFY(ar.saleId > 0);

    Check ch = sm.getCheck(cr.checkId);
    QVERIFY(qAbs(ch.totalAmount - 200.0) < 0.01);

    QList<SaleRow> rows = sm.getSalesByCheck(cr.checkId);
    QVERIFY(rows.size() == 1);
    QVERIFY(rows[0].qnt == 2);
    QVERIFY(qAbs(rows[0].sum - 200.0) < 0.01);
}

void TestSalesManager::testAddSaleByServiceId()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    qint64 empId = createTestEmployee();
    qint64 serviceId = createTestService(QString(), 50.0);
    if (empId <= 0 || serviceId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);

    SaleOperationResult ar = sm.addSaleByServiceId(cr.checkId, serviceId, 3);
    QVERIFY(ar.success);

    Check ch = sm.getCheck(cr.checkId);
    QVERIFY(qAbs(ch.totalAmount - 150.0) < 0.01);

    QList<SaleRow> rows = sm.getSalesByCheck(cr.checkId);
    QVERIFY(rows.size() == 1);
    QVERIFY(rows[0].qnt == 3);
    QVERIFY(qAbs(rows[0].sum - 150.0) < 0.01);
}

void TestSalesManager::testAddSaleInsufficientBatch()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    int goodId = createTestGood();
    qint64 empId = createTestEmployee();
    if (goodId <= 0 || empId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    qint64 batchId = createTestBatch(goodId, 10.0, 1);
    QVERIFY(batchId > 0);

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);

    SaleOperationResult ar = sm.addSaleByBatchId(cr.checkId, batchId, 100);
    QVERIFY(!ar.success);
    QVERIFY(ar.message.contains("Недостаточно"));
}

void TestSalesManager::testRemoveSale()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    int goodId = createTestGood();
    qint64 empId = createTestEmployee();
    qint64 batchId = createTestBatch(goodId, 25.0, 5);
    if (goodId <= 0 || empId <= 0 || batchId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);
    SaleOperationResult ar = sm.addSaleByBatchId(cr.checkId, batchId, 2);
    QVERIFY(ar.success);
    QVERIFY(ar.saleId > 0);

    SaleOperationResult rr = sm.removeSale(ar.saleId);
    QVERIFY(rr.success);

    Check ch = sm.getCheck(cr.checkId);
    QVERIFY(qAbs(ch.totalAmount - 0.0) < 0.01);

    QList<SaleRow> rows = sm.getSalesByCheck(cr.checkId);
    QVERIFY(rows.isEmpty());
}

void TestSalesManager::testUpdateSaleQuantity()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    int goodId = createTestGood();
    qint64 empId = createTestEmployee();
    qint64 batchId = createTestBatch(goodId, 30.0, 20);
    if (goodId <= 0 || empId <= 0 || batchId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);
    SaleOperationResult ar = sm.addSaleByBatchId(cr.checkId, batchId, 3);
    QVERIFY(ar.success);

    SaleOperationResult ur = sm.updateSaleQuantity(ar.saleId, 5);
    QVERIFY(ur.success);

    QList<SaleRow> rows = sm.getSalesByCheck(cr.checkId);
    QVERIFY(rows.size() == 1);
    QVERIFY(rows[0].qnt == 5);
    QVERIFY(qAbs(rows[0].sum - 150.0) < 0.01);

    Check ch = sm.getCheck(cr.checkId);
    QVERIFY(qAbs(ch.totalAmount - 150.0) < 0.01);
}

void TestSalesManager::testSetCheckDiscount()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    qint64 empId = createTestEmployee();
    qint64 serviceId = createTestService(QString(), 100.0);
    if (empId <= 0 || serviceId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);
    sm.addSaleByServiceId(cr.checkId, serviceId, 1);

    SaleOperationResult dr = sm.setCheckDiscount(cr.checkId, 15.0);
    QVERIFY(dr.success);

    Check ch = sm.getCheck(cr.checkId);
    QVERIFY(qAbs(ch.totalAmount - 100.0) < 0.01);
    QVERIFY(qAbs(ch.discountAmount - 15.0) < 0.01);
}

void TestSalesManager::testCancelCheck()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    int goodId = createTestGood();
    qint64 empId = createTestEmployee();
    qint64 batchId = createTestBatch(goodId, 10.0, 5);
    if (goodId <= 0 || empId <= 0 || batchId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);
    sm.addSaleByBatchId(cr.checkId, batchId, 2);

    SaleOperationResult cancr = sm.cancelCheck(cr.checkId);
    QVERIFY(cancr.success);

    Check ch = sm.getCheck(cr.checkId, true);
    QVERIFY(ch.isDeleted);
}

void TestSalesManager::testGetSalesByCheck()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    int goodId = createTestGood();
    qint64 empId = createTestEmployee();
    qint64 batchId = createTestBatch(goodId, 20.0, 10);
    qint64 serviceId = createTestService(QString(), 5.0);
    if (goodId <= 0 || empId <= 0 || batchId <= 0 || serviceId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);
    sm.addSaleByBatchId(cr.checkId, batchId, 1);
    sm.addSaleByServiceId(cr.checkId, serviceId, 2);

    QList<SaleRow> rows = sm.getSalesByCheck(cr.checkId);
    QVERIFY(rows.size() == 2);
}

void TestSalesManager::testGetSale()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    qint64 empId = createTestEmployee();
    qint64 serviceId = createTestService(QString(), 77.0);
    if (empId <= 0 || serviceId <= 0) {
        QSKIP("Не удалось создать тестовые данные");
        return;
    }

    SaleOperationResult cr = sm.createCheck(empId);
    QVERIFY(cr.success);
    SaleOperationResult ar = sm.addSaleByServiceId(cr.checkId, serviceId, 2);
    QVERIFY(ar.success);

    SaleRow row = sm.getSale(ar.saleId);
    QVERIFY(row.id == ar.saleId);
    QVERIFY(row.qnt == 2);
    QVERIFY(qAbs(row.sum - 154.0) < 0.01);
    QVERIFY(!row.itemName.isEmpty());
}

void TestSalesManager::testGetChecks()
{
    SalesManager sm;
    DatabaseConnection *dbConn = TestHelpers::createTestDatabaseConnection();
    if (!dbConn || !dbConn->isConnected()) {
        QSKIP("Нет подключения к БД для теста");
        return;
    }
    sm.setDatabaseConnection(dbConn);

    qint64 empId = createTestEmployee();
    if (empId <= 0) {
        QSKIP("Не удалось создать тестового сотрудника");
        return;
    }

    sm.createCheck(empId);
    sm.createCheck(empId);

    QList<Check> checks = sm.getChecks(QDate::currentDate(), QDate::currentDate(), empId, false);
    QVERIFY(checks.size() >= 2);
}
