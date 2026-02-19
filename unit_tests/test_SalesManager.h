#ifndef TEST_SALESMANAGER_H
#define TEST_SALESMANAGER_H

#include <QtTest>
#include <QObject>
#include "../sales/salesmanager.h"
#include "../sales/stockmanager.h"
#include "../db/databaseconnection.h"
#include "../db/structures.h"
#include "test_helpers.h"

class TestSalesManager : public QObject
{
    Q_OBJECT

public:
    TestSalesManager();
    ~TestSalesManager();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstructor();
    void testSetDatabaseConnection();
    void testCreateCheck();
    void testCreateCheckWithoutConnection();
    void testCreateCheckInvalidEmployee();
    void testGetCheck();
    void testAddSaleByBatchId();
    void testAddSaleByServiceId();
    void testAddSaleInsufficientBatch();
    void testRemoveSale();
    void testUpdateSaleQuantity();
    void testSetCheckDiscount();
    void testCancelCheck();
    void testGetSalesByCheck();
    void testGetSale();
    void testGetChecks();

private:
    qint64 createTestEmployee();
    qint64 createTestBatch(int goodId, double price, qint64 qnt);
    qint64 createTestService(const QString &name, double price);
    int createTestGood(const QString &name = QString());
    void cleanupTestData();
};

#endif // TEST_SALESMANAGER_H
