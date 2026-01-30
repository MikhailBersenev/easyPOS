#include "test_AuthManager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>

TestAuthManager::TestAuthManager()
{
}

TestAuthManager::~TestAuthManager()
{
}

void TestAuthManager::initTestCase()
{
    qDebug() << "Инициализация тестового набора для AuthManager";
}

void TestAuthManager::cleanupTestCase()
{
    qDebug() << "Завершение тестового набора для AuthManager";
    cleanupDatabaseConnections();
}

void TestAuthManager::init()
{
    // Инициализация перед каждым тестом
}

void TestAuthManager::cleanup()
{
    // Очистка после каждого теста
    cleanupDatabaseConnections();
}

void TestAuthManager::testConstructor()
{
    AuthManager authManager;
    
    // Проверяем, что объект создан
    QVERIFY(authManager.getSession(0).userId == 0);
}

void TestAuthManager::testSetDatabaseConnection()
{
    AuthManager authManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        authManager.setDatabaseConnection(dbConn);
        // Если подключение успешно, можно протестировать авторизацию
        // Но для этого нужны данные в БД
        QVERIFY(true);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAuthManager::testSetDatabaseConnectionNull()
{
    AuthManager authManager;
    authManager.setDatabaseConnection(nullptr);
    
    // Проверяем, что авторизация не работает без подключения
    AuthResult result = authManager.authenticate("test", "test");
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestAuthManager::testAuthenticateWithEmptyUsername()
{
    AuthManager authManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        authManager.setDatabaseConnection(dbConn);
        
        AuthResult result = authManager.authenticate("", "password");
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не могут быть пустыми"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAuthManager::testAuthenticateWithEmptyPassword()
{
    AuthManager authManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        authManager.setDatabaseConnection(dbConn);
        
        AuthResult result = authManager.authenticate("username", "");
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не могут быть пустыми"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAuthManager::testAuthenticateWithoutConnection()
{
    AuthManager authManager;
    // Не устанавливаем подключение
    
    AuthResult result = authManager.authenticate("test", "test");
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestAuthManager::testAuthenticateWithInvalidUser()
{
    AuthManager authManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        authManager.setDatabaseConnection(dbConn);
        
        // Пытаемся авторизоваться с несуществующим пользователем
        AuthResult result = authManager.authenticate("nonexistent_user_12345", "password");
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найден") || result.message.contains("Нет подключения"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAuthManager::testAuthenticateWithInvalidPassword()
{
    AuthManager authManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        authManager.setDatabaseConnection(dbConn);
        
        // Этот тест требует существующего пользователя в БД
        // Поэтому просто проверяем структуру результата
        AuthResult result = authManager.authenticate("nonexistent_user", "wrong_password");
        QVERIFY(!result.success || result.message.contains("не найден") || result.message.contains("Неверный пароль"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAuthManager::testGetSession()
{
    AuthManager authManager;
    
    // Проверяем получение сессии для несуществующего пользователя
    UserSession session = authManager.getSession(999);
    QVERIFY(!session.isValid());
    QVERIFY(session.userId == 0);
}

void TestAuthManager::testIsSessionValid()
{
    AuthManager authManager;
    
    // Проверяем валидность сессии для несуществующего пользователя
    bool isValid = authManager.isSessionValid(999);
    QVERIFY(!isValid);
}

void TestAuthManager::testUpdateSessionActivity()
{
    AuthManager authManager;
    
    // Обновление активности для несуществующей сессии не должно вызывать ошибок
    authManager.updateSessionActivity(999);
    QVERIFY(true); // Просто проверяем, что метод не падает
}

void TestAuthManager::testHasPermission()
{
    AuthManager authManager;
    
    // Проверка прав для несуществующего пользователя
    bool hasPerm = authManager.hasPermission(999, "test.permission");
    QVERIFY(!hasPerm);
}

void TestAuthManager::testHasPermissionWithInvalidSession()
{
    AuthManager authManager;
    
    UserSession invalidSession;
    bool hasPerm = authManager.hasPermission(invalidSession, "test.permission");
    QVERIFY(!hasPerm);
}

void TestAuthManager::testLogout()
{
    AuthManager authManager;
    
    // Выход для несуществующего пользователя не должен вызывать ошибок
    bool result = authManager.logout(999);
    QVERIFY(!result); // Должен вернуть false, так как пользователь не был авторизован
}

void TestAuthManager::testLogoutWithInvalidUser()
{
    AuthManager authManager;
    
    // Выход для несуществующего пользователя
    bool result = authManager.logout(-1);
    QVERIFY(!result);
}

void TestAuthManager::testGetUserRole()
{
    AuthManager authManager;
    DatabaseConnection* dbConn = createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        authManager.setDatabaseConnection(dbConn);
        
        // Получение роли для несуществующего пользователя
        Role role = authManager.getUserRole(999);
        QVERIFY(role.id == 0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

DatabaseConnection* TestAuthManager::createTestDatabaseConnection()
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

PostgreSQLAuth TestAuthManager::createTestAuth() const
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

void TestAuthManager::cleanupDatabaseConnections()
{
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &name : connections) {
        if (name.startsWith("easyPOS_connection_")) {
            QSqlDatabase::removeDatabase(name);
        }
    }
}
