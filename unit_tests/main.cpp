#include "test_databaseconnection.h"
#include "test_AuthManager.h"
#include "test_AccountManager.h"
#include "test_RoleManager.h"
#include "test_StockManager.h"
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
    
    qDebug() << "\n=== Итоги тестирования ===";
    if (failures == 0) {
        qDebug() << "Все тесты пройдены успешно!";
    } else {
        qDebug() << "Обнаружено ошибок:" << failures;
    }
    
    return failures;
}

