#include "test_RoleManager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QDateTime>

TestRoleManager::TestRoleManager()
{
}

TestRoleManager::~TestRoleManager()
{
}

void TestRoleManager::initTestCase()
{
    qDebug() << "Инициализация тестового набора для RoleManager";
}

void TestRoleManager::cleanupTestCase()
{
    qDebug() << "Завершение тестового набора для RoleManager";
    cleanupDatabaseConnections();
}

void TestRoleManager::init()
{
}

void TestRoleManager::cleanup()
{
    cleanupDatabaseConnections();
}

void TestRoleManager::testConstructor()
{
    RoleManager roleManager;
    QVERIFY(!roleManager.roleExists("nonexistent_role"));
}

void TestRoleManager::testSetDatabaseConnection()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn) {
        roleManager.setDatabaseConnection(dbConn);
        QVERIFY(true);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testSetDatabaseConnectionNull()
{
    RoleManager roleManager;
    roleManager.setDatabaseConnection(nullptr);
    QVERIFY(!roleManager.roleExists("any_role"));
}

void TestRoleManager::testCreateRoleWithEmptyName()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        RoleOperationResult result = roleManager.createRole("", 0);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не может быть пустым"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testCreateRoleWithoutConnection()
{
    RoleManager roleManager;
    
    RoleOperationResult result = roleManager.createRole("test", 0);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestRoleManager::testCreateRole()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult result = roleManager.createRole(roleName, 1);
        
        QVERIFY(result.success);
        QVERIFY(result.roleId > 0);
        QVERIFY(roleManager.roleExists(roleName));
        
        Role role = roleManager.getRole(result.roleId);
        QVERIFY(role.id == result.roleId);
        QVERIFY(role.name == roleName);
        QVERIFY(role.level == 1);
        QVERIFY(role.isActive);
        QVERIFY(!role.isBlocked);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testCreateRoleDuplicate()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        
        RoleOperationResult result1 = roleManager.createRole(roleName, 1);
        QVERIFY(result1.success);
        
        RoleOperationResult result2 = roleManager.createRole(roleName, 1);
        QVERIFY(!result2.success);
        QVERIFY(result2.message.contains("уже существует"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testGetRole()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 2);
        QVERIFY(createResult.success);
        
        Role role = roleManager.getRole(createResult.roleId);
        QVERIFY(role.id == createResult.roleId);
        QVERIFY(role.name == roleName);
        QVERIFY(role.level == 2);
        QVERIFY(role.isActive);
        
        Role nonExistent = roleManager.getRole(99999);
        QVERIFY(nonExistent.id == 0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testGetRoleByName()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 3);
        QVERIFY(createResult.success);
        
        Role role = roleManager.getRole(roleName);
        QVERIFY(role.id == createResult.roleId);
        QVERIFY(role.name == roleName);
        QVERIFY(role.level == 3);
        
        Role nonExistent = roleManager.getRole("nonexistent_role_12345");
        QVERIFY(nonExistent.id == 0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testGetAllRoles()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName1 = generateUniqueRoleName();
        QString roleName2 = generateUniqueRoleName();
        
        roleManager.createRole(roleName1, 1);
        roleManager.createRole(roleName2, 2);
        
        QList<Role> roles = roleManager.getAllRoles();
        QVERIFY(roles.size() >= 2);
        
        bool found1 = false, found2 = false;
        for (const Role &role : roles) {
            if (role.name == roleName1) found1 = true;
            if (role.name == roleName2) found2 = true;
        }
        QVERIFY(found1);
        QVERIFY(found2);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testRoleExists()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        
        QVERIFY(!roleManager.roleExists(roleName));
        
        RoleOperationResult result = roleManager.createRole(roleName, 1);
        QVERIFY(result.success);
        
        QVERIFY(roleManager.roleExists(roleName));
        QVERIFY(roleManager.roleExists(result.roleId));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testUpdateRole()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        
        QString newName = generateUniqueRoleName();
        RoleOperationResult updateResult = roleManager.updateRole(createResult.roleId, newName, 2);
        QVERIFY(updateResult.success);
        
        Role role = roleManager.getRole(createResult.roleId);
        QVERIFY(role.name == newName);
        QVERIFY(role.level == 2);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testUpdateRoleWithoutConnection()
{
    RoleManager roleManager;
    
    RoleOperationResult result = roleManager.updateRole(1, "test", 0);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestRoleManager::testUpdateRoleNotFound()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        RoleOperationResult result = roleManager.updateRole(99999, "test", 0);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найдена"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testDeleteRole()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        QVERIFY(roleManager.roleExists(roleName));
        
        RoleOperationResult deleteResult = roleManager.deleteRole(createResult.roleId);
        QVERIFY(deleteResult.success);
        
        QVERIFY(!roleManager.roleExists(roleName, false));
        QVERIFY(roleManager.roleExists(roleName, true));
        
        Role deletedRole = roleManager.getRole(createResult.roleId, true);
        QVERIFY(deletedRole.id == createResult.roleId);
        QVERIFY(!deletedRole.isActive);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testDeleteRoleByName()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        
        RoleOperationResult deleteResult = roleManager.deleteRole(roleName);
        QVERIFY(deleteResult.success);
        QVERIFY(!roleManager.roleExists(roleName, false));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testDeleteRoleWithoutConnection()
{
    RoleManager roleManager;
    
    RoleOperationResult result = roleManager.deleteRole(1);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestRoleManager::testRestoreRole()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        
        roleManager.deleteRole(createResult.roleId);
        QVERIFY(!roleManager.roleExists(roleName, false));
        
        RoleOperationResult restoreResult = roleManager.restoreRole(createResult.roleId);
        QVERIFY(restoreResult.success);
        
        QVERIFY(roleManager.roleExists(roleName, false));
        
        Role restoredRole = roleManager.getRole(createResult.roleId);
        QVERIFY(restoredRole.isActive);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testRestoreRoleByName()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        
        roleManager.deleteRole(roleName);
        
        RoleOperationResult restoreResult = roleManager.restoreRole(roleName);
        QVERIFY(restoreResult.success);
        QVERIFY(roleManager.roleExists(roleName, false));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testRestoreRoleWithoutConnection()
{
    RoleManager roleManager;
    
    RoleOperationResult result = roleManager.restoreRole(1);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestRoleManager::testBlockRole()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        QVERIFY(!roleManager.getRole(createResult.roleId).isBlocked);
        
        RoleOperationResult blockResult = roleManager.blockRole(createResult.roleId, true);
        QVERIFY(blockResult.success);
        
        Role role = roleManager.getRole(createResult.roleId);
        QVERIFY(role.isBlocked);
        
        RoleOperationResult unblockResult = roleManager.blockRole(createResult.roleId, false);
        QVERIFY(unblockResult.success);
        
        role = roleManager.getRole(createResult.roleId);
        QVERIFY(!role.isBlocked);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testBlockRoleByName()
{
    RoleManager roleManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        roleManager.setDatabaseConnection(dbConn);
        
        QString roleName = generateUniqueRoleName();
        RoleOperationResult createResult = roleManager.createRole(roleName, 1);
        QVERIFY(createResult.success);
        
        RoleOperationResult blockResult = roleManager.blockRole(roleName, true);
        QVERIFY(blockResult.success);
        
        Role role = roleManager.getRole(roleName);
        QVERIFY(role.isBlocked);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestRoleManager::testBlockRoleWithoutConnection()
{
    RoleManager roleManager;
    
    RoleOperationResult result = roleManager.blockRole(1, true);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

// === Вспомогательные методы ===

DatabaseConnection* TestRoleManager::createTestDatabaseConnection()
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

PostgreSQLAuth TestRoleManager::createTestAuth() const
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

void TestRoleManager::cleanupDatabaseConnections()
{
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &name : connections) {
        if (name.startsWith("easyPOS_connection_")) {
            QSqlDatabase::removeDatabase(name);
        }
    }
}

QString TestRoleManager::generateUniqueRoleName() const
{
    return QString("test_role_%1").arg(QDateTime::currentMSecsSinceEpoch());
}
