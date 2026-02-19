#include "test_databaseconnection.h"
#include "test_AuthManager.h"
#include "test_AccountManager.h"
#include "test_RoleManager.h"
#include "test_StockManager.h"
#include "test_SalesManager.h"
#include "test_SettingsManager.h"
#include "test_ShiftManager.h"
#include "test_AlertsManager.h"
#include "test_ReportManager.h"
#include "test_ProductionManager.h"
#include "test_EasyPOSCore.h"
#include "test_CategoryManager.h"
#include "test_PositionManager.h"
#include "test_EmployeeManager.h"
#include "test_PromotionManager.h"
#include <QtTest>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    int failures = 0;
    
    // Запуск тестов DatabaseConnection
    qDebug() << "\n=== Тестирование DatabaseConnection ===";
    TestDatabaseConnection testDatabaseConnection;
    failures += QTest::qExec(&testDatabaseConnection, argc, argv);
    
    // Запуск тестов AuthManager
    qDebug() << "\n=== Тестирование AuthManager ===";
    TestAuthManager testAuthManager;
    failures += QTest::qExec(&testAuthManager, argc, argv);
    
    // Запуск тестов AccountManager
    qDebug() << "\n=== Тестирование AccountManager ===";
    TestAccountManager testAccountManager;
    failures += QTest::qExec(&testAccountManager, argc, argv);
    
    // Запуск тестов RoleManager
    qDebug() << "\n=== Тестирование RoleManager ===";
    TestRoleManager testRoleManager;
    failures += QTest::qExec(&testRoleManager, argc, argv);
    
    // Запуск тестов StockManager
    qDebug() << "\n=== Тестирование StockManager ===";
    TestStockManager testStockManager;
    failures += QTest::qExec(&testStockManager, argc, argv);
    
    // Запуск тестов SalesManager
    qDebug() << "\n=== Тестирование SalesManager ===";
    TestSalesManager testSalesManager;
    failures += QTest::qExec(&testSalesManager, argc, argv);
    
    // Запуск тестов SettingsManager
    qDebug() << "\n=== Тестирование SettingsManager ===";
    TestSettingsManager testSettingsManager;
    failures += QTest::qExec(&testSettingsManager, argc, argv);
    
    // Запуск тестов ShiftManager
    qDebug() << "\n=== Тестирование ShiftManager ===";
    TestShiftManager testShiftManager;
    failures += QTest::qExec(&testShiftManager, argc, argv);
    
    // Запуск тестов AlertsManager
    qDebug() << "\n=== Тестирование AlertsManager ===";
    TestAlertsManager testAlertsManager;
    failures += QTest::qExec(&testAlertsManager, argc, argv);
    
    // Запуск тестов ReportManager
    qDebug() << "\n=== Тестирование ReportManager ===";
    TestReportManager testReportManager;
    failures += QTest::qExec(&testReportManager, argc, argv);
    
    // Запуск тестов ProductionManager
    qDebug() << "\n=== Тестирование ProductionManager ===";
    TestProductionManager testProductionManager;
    failures += QTest::qExec(&testProductionManager, argc, argv);
    
    // Запуск тестов EasyPOSCore
    qDebug() << "\n=== Тестирование EasyPOSCore ===";
    TestEasyPOSCore testEasyPOSCore;
    failures += QTest::qExec(&testEasyPOSCore, argc, argv);
    
    // Запуск тестов CategoryManager
    qDebug() << "\n=== Тестирование CategoryManager ===";
    TestCategoryManager testCategoryManager;
    failures += QTest::qExec(&testCategoryManager, argc, argv);
    
    // Запуск тестов PositionManager
    qDebug() << "\n=== Тестирование PositionManager ===";
    TestPositionManager testPositionManager;
    failures += QTest::qExec(&testPositionManager, argc, argv);
    
    // Запуск тестов EmployeeManager
    qDebug() << "\n=== Тестирование EmployeeManager ===";
    TestEmployeeManager testEmployeeManager;
    failures += QTest::qExec(&testEmployeeManager, argc, argv);
    
    // Запуск тестов PromotionManager
    qDebug() << "\n=== Тестирование PromotionManager ===";
    TestPromotionManager testPromotionManager;
    failures += QTest::qExec(&testPromotionManager, argc, argv);
    
    qDebug() << "\n=== Итоги тестирования ===";
    if (failures == 0) {
        qDebug() << "Все тесты пройдены успешно!";
    } else {
        qDebug() << "Обнаружено ошибок:" << failures;
    }
    
    return failures;
}

