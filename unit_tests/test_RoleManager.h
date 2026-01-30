#ifndef TEST_ROLEMANAGER_H
#define TEST_ROLEMANAGER_H

#include <QtTest>
#include <QObject>
#include "../RBAC/rolemanager.h"
#include "../db/databaseconnection.h"
#include "../db/structures.h"

class TestRoleManager : public QObject
{
    Q_OBJECT

public:
    TestRoleManager();
    ~TestRoleManager();

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
    
    // === Тесты управления ролями ===
    
    // Тесты создания ролей
    void testCreateRoleWithEmptyName();
    void testCreateRoleWithoutConnection();
    void testCreateRole();
    void testCreateRoleDuplicate();
    
    // Тесты получения ролей
    void testGetRole();
    void testGetRoleByName();
    void testGetAllRoles();
    void testRoleExists();
    
    // Тесты обновления ролей
    void testUpdateRole();
    void testUpdateRoleWithoutConnection();
    void testUpdateRoleNotFound();
    
    // Тесты удаления ролей
    void testDeleteRole();
    void testDeleteRoleByName();
    void testDeleteRoleWithoutConnection();
    
    // Тесты восстановления ролей
    void testRestoreRole();
    void testRestoreRoleByName();
    void testRestoreRoleWithoutConnection();
    
    // Тесты блокировки ролей
    void testBlockRole();
    void testBlockRoleByName();
    void testBlockRoleWithoutConnection();

private:
    DatabaseConnection* createTestDatabaseConnection();
    PostgreSQLAuth createTestAuth() const;
    void cleanupDatabaseConnections();
    QString generateUniqueRoleName() const;
};

#endif // TEST_ROLEMANAGER_H

