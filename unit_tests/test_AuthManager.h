#ifndef TEST_AUTHMANAGER_H
#define TEST_AUTHMANAGER_H

#include <QtTest>
#include <QObject>
#include "../RBAC/authmanager.h"
#include "../RBAC/structures.h"
#include "../db/databaseconnection.h"
#include "../db/structures.h"

class TestAuthManager : public QObject
{
    Q_OBJECT

public:
    TestAuthManager();
    ~TestAuthManager();

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
    
    // Тесты авторизации
    void testAuthenticateWithEmptyUsername();
    void testAuthenticateWithEmptyPassword();
    void testAuthenticateWithoutConnection();
    void testAuthenticateWithInvalidUser();
    void testAuthenticateWithInvalidPassword();
    
    // Тесты сессий
    void testGetSession();
    void testIsSessionValid();
    void testUpdateSessionActivity();
    
    // Тесты прав доступа
    void testHasPermission();
    void testHasPermissionWithInvalidSession();
    
    // Тесты выхода
    void testLogout();
    void testLogoutWithInvalidUser();
    
    // Тесты получения роли
    void testGetUserRole();

private:
    DatabaseConnection* createTestDatabaseConnection();
    PostgreSQLAuth createTestAuth() const;
    void cleanupDatabaseConnections();
};

#endif // TEST_AUTHMANAGER_H
