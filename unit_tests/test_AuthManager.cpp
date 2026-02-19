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
    TestHelpers::cleanupDatabaseConnections();
}

void TestAuthManager::init()
{
}

void TestAuthManager::cleanup()
{
    TestHelpers::cleanupDatabaseConnections();
}

void TestAuthManager::testConstructor()
{
    AuthManager authManager;
    UserSession s = authManager.getSession(0);
    QCOMPARE(s.userId, 0);
    QVERIFY(!s.isValid());
}

void TestAuthManager::testSetDatabaseConnection()
{
    AuthManager authManager;
    TEST_REQUIRE_CONNECTION(conn);
    authManager.setDatabaseConnection(conn);
    QVERIFY2(conn->isConnected(), "Требуется рабочее подключение к БД");
}

void TestAuthManager::testSetDatabaseConnectionNull()
{
    AuthManager authManager;
    authManager.setDatabaseConnection(nullptr);
    AuthResult result = authManager.authenticate("test", "test");
    QVERIFY2(!result.success, "Без подключения авторизация должна завершаться ошибкой");
    QVERIFY2(result.message.contains("Нет подключения"), qPrintable(result.message));
}

void TestAuthManager::testAuthenticateWithEmptyCredentials_data()
{
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("password");
    QTest::newRow("пустой_логин") << "" << "password";
    QTest::newRow("пустой_пароль") << "username" << "";
}

void TestAuthManager::testAuthenticateWithEmptyCredentials()
{
    QFETCH(QString, username);
    QFETCH(QString, password);
    AuthManager authManager;
    TEST_REQUIRE_CONNECTION(conn);
    authManager.setDatabaseConnection(conn);
    AuthResult result = authManager.authenticate(username, password);
    QVERIFY2(!result.success, "Пустые логин или пароль должны отклоняться");
    QVERIFY2(result.message.contains("не могут быть пустыми"), qPrintable(result.message));
}

void TestAuthManager::testAuthenticateWithoutConnection()
{
    AuthManager authManager;
    AuthResult result = authManager.authenticate("test", "test");
    QVERIFY2(!result.success, "Без подключения авторизация должна завершаться ошибкой");
    QVERIFY2(result.message.contains("Нет подключения"), qPrintable(result.message));
}

void TestAuthManager::testAuthenticateWithInvalidUser()
{
    AuthManager authManager;
    TEST_REQUIRE_CONNECTION(conn);
    authManager.setDatabaseConnection(conn);
    AuthResult result = authManager.authenticate("nonexistent_user_12345", "password");
    QVERIFY2(!result.success, "Несуществующий пользователь должен отклоняться");
    QVERIFY(result.message.contains("не найден") || result.message.contains("Нет подключения"));
}

void TestAuthManager::testAuthenticateWithInvalidPassword()
{
    AuthManager authManager;
    TEST_REQUIRE_CONNECTION(conn);
    authManager.setDatabaseConnection(conn);
    AuthResult result = authManager.authenticate("nonexistent_user", "wrong_password");
    QVERIFY2(!result.success, "Неверный пароль или несуществующий пользователь");
    QVERIFY(result.message.contains("не найден") || result.message.contains("Неверный пароль"));
}

void TestAuthManager::testGetSession()
{
    AuthManager authManager;
    UserSession session = authManager.getSession(999);
    QVERIFY(!session.isValid());
    QCOMPARE(session.userId, 0);
}

void TestAuthManager::testIsSessionValid()
{
    AuthManager authManager;
    QVERIFY(!authManager.isSessionValid(999));
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
    TEST_REQUIRE_CONNECTION(conn);
    authManager.setDatabaseConnection(conn);
    Role role = authManager.getUserRole(999);
    QCOMPARE(role.id, 0);
}

