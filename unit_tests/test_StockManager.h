#ifndef TEST_STOCKMANAGER_H
#define TEST_STOCKMANAGER_H

#include <QtTest>
#include <QObject>
#include "../sales/stockmanager.h"
#include "../db/databaseconnection.h"
#include "../db/structures.h"
#include "../easyposcore.h"

class TestStockManager : public QObject
{
    Q_OBJECT

public:
    TestStockManager();
    ~TestStockManager();

private slots:
    // Инициализация и очистка
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Тесты конструктора
    void testConstructor();
    
    // Тесты установки подключения
    void testSetDatabaseConnection();
    void testSetDatabaseConnectionNull();
    
    // === Тесты управления остатками ===
    
    // Тесты установки остатка
    void testSetStock();
    void testSetStockByGoodName();
    void testSetStockWithoutConnection();
    void testSetStockInvalidGood();
    void testSetStockNegativeQuantity();
    
    // Тесты добавления остатка
    void testAddStock();
    void testAddStockByGoodName();
    void testAddStockMultipleBatches();
    
    // Тесты получения остатка
    void testGetStockByGoodId();
    void testGetStockByGoodName();
    void testGetAllStocks();
    void testGetStocksByGoodId();
    
    // Тесты списания
    void testRemoveStock();
    void testRemoveStockByGoodName();
    void testRemoveStockInsufficient();
    void testRemoveStockFIFO();
    
    // Тесты резервирования
    void testReserveStock();
    void testReserveStockByGoodName();
    void testReserveStockInsufficient();
    void testReleaseReserve();
    void testReleaseReserveInsufficient();
    
    // Тесты проверки наличия
    void testHasEnoughStock();
    void testHasEnoughStockByGoodName();
    void testStockExists();
    
    // Тесты удаления и восстановления
    void testDeleteStock();
    void testRestoreStock();

private:
    DatabaseConnection* createTestDatabaseConnection();
    PostgreSQLAuth createTestAuth() const;
    void cleanupDatabaseConnections();
    int createTestGood(const QString &name = QString());
    void cleanupTestData();
};

#endif // TEST_STOCKMANAGER_H
