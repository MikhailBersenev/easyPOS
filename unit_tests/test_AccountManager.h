#ifndef TEST_ACCOUNTMANAGER_H
#define TEST_ACCOUNTMANAGER_H

#include <QtTest>
#include <QObject>
#include "../RBAC/accountmanager.h"
#include "../db/databaseconnection.h"
#include "../db/structures.h"
#include "../RBAC/structures.h"

class TestAccountManager : public QObject
{
    Q_OBJECT

public:
    TestAccountManager();
    ~TestAccountManager();

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
    
    // Тесты регистрации пользователей
    void testRegisterUserWithEmptyUsername();
    void testRegisterUserWithEmptyPassword();
    void testRegisterUserWithoutConnection();
    void testRegisterUserWithInvalidRole();
    
    // Тесты проверки существования пользователя
    void testUserExists();
    void testUserExistsWithIncludeDeleted();
    
    // Тесты получения пользователя
    void testGetUser();
    void testGetUserByUsername();
    void testGetAllUsers();
    
    // Тесты удаления пользователя
    void testDeleteUser();
    void testDeleteUserByUsername();
    void testDeleteUserWithoutConnection();
    
    // Тесты восстановления пользователя
    void testRestoreUser();
    void testRestoreUserByUsername();
    void testRestoreUserWithoutConnection();

private:
    DatabaseConnection* createTestDatabaseConnection();
    PostgreSQLAuth createTestAuth() const;
    void cleanupDatabaseConnections();
    QString generateUniqueUsername() const;
};

#endif // TEST_ACCOUNTMANAGER_H

