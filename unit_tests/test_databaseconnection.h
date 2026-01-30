#ifndef TEST_DATABASECONNECTION_H
#define TEST_DATABASECONNECTION_H

#include <QtTest>
#include <QObject>
#include "../db/databaseconnection.h"
#include "../db/structures.h"

class TestDatabaseConnection : public QObject
{
    Q_OBJECT

public:
    TestDatabaseConnection();
    ~TestDatabaseConnection();

private slots:
    // Инициализация и очистка
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Тесты конструктора и деструктора
    void testConstructor();
    void testDestructor();

    // Тесты валидации данных авторизации
    void testInvalidAuthData();
    void testValidAuthData();
    void testEmptyHost();
    void testEmptyDatabase();
    void testEmptyUsername();
    void testInvalidPort();

    // Тесты подключения
    void testConnectWithInvalidData();
    void testConnectWithoutDriver();
    void testDisconnect();
    void testIsConnected();

    // Тесты получения данных
    void testGetDatabase();
    void testLastError();
    void testConnectionName();

    // Тесты повторного подключения
    void testReconnect();

private:
    PostgreSQLAuth createValidAuth() const;
    PostgreSQLAuth createInvalidAuth() const;
};

#endif // TEST_DATABASECONNECTION_H

