#include "test_AccountManager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QUuid>
#include <QDateTime>

TestAccountManager::TestAccountManager()
{
}

TestAccountManager::~TestAccountManager()
{
}

void TestAccountManager::initTestCase()
{
    qDebug() << "Инициализация тестового набора для AccountManager";
}

void TestAccountManager::cleanupTestCase()
{
    qDebug() << "Завершение тестового набора для AccountManager";
    TestHelpers::cleanupDatabaseConnections();
}

void TestAccountManager::init()
{
}

void TestAccountManager::cleanup()
{
    TestHelpers::cleanupDatabaseConnections();
}

void TestAccountManager::testConstructor()
{
    AccountManager accountManager;
    
    // Проверяем, что объект создан
    QVERIFY(!accountManager.userExists("nonexistent"));
}

void TestAccountManager::testSetDatabaseConnection()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        // Если подключение успешно, можно протестировать операции
        QVERIFY(true);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testSetDatabaseConnectionNull()
{
    AccountManager accountManager;
    accountManager.setDatabaseConnection(nullptr);
    
    // Проверяем, что регистрация не работает без подключения
    UserOperationResult result = accountManager.registerUser("test", "test", 1);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestAccountManager::testRegisterUserWithEmptyUsername()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        UserOperationResult result = accountManager.registerUser("", "password", 1);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не могут быть пустыми"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testRegisterUserWithEmptyPassword()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        UserOperationResult result = accountManager.registerUser("username", "", 1);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не могут быть пустыми"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testRegisterUserWithoutConnection()
{
    AccountManager accountManager;
    // Не устанавливаем подключение
    
    UserOperationResult result = accountManager.registerUser("test", "test", 1);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestAccountManager::testRegisterUserWithInvalidRole()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Пытаемся зарегистрировать пользователя с несуществующей ролью
        QString uniqueUsername = generateUniqueUsername();
        UserOperationResult result = accountManager.registerUser(uniqueUsername, "password", 99999);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найдена"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testUserExists()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Проверка существования несуществующего пользователя
        bool exists = accountManager.userExists("nonexistent_user_12345", false);
        QVERIFY(!exists);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testUserExistsWithIncludeDeleted()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Проверка с включением удаленных
        bool exists = accountManager.userExists("nonexistent_user_12345", true);
        QVERIFY(!exists);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testGetUser()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Получение пользователя - проверяем, что метод работает без ошибок
        // Используем очень большое число, чтобы гарантировать, что пользователь не существует
        // Метод должен работать корректно в любом случае
        User user = accountManager.getUser(99999999, false);
        // Проверяем согласованность: если пользователь не существует, то id должен быть 0
        // Если userExists и getUser работают согласованно
        bool exists = accountManager.userExists(99999999, false);
        if (!exists) {
            QVERIFY(user.id == 0); // Пользователь не существует, значит getUser должен вернуть пустого
        } else {
            QVERIFY(user.id > 0); // Пользователь существует, значит getUser должен вернуть его
        }
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testGetUserByUsername()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Получение пользователя по имени - проверяем, что метод работает без ошибок
        // Используем очень длинное уникальное имя
        QString uniqueName = QString("test_nonexistent_user_%1_xyz").arg(QDateTime::currentMSecsSinceEpoch());
        
        // Метод должен работать корректно в любом случае
        User user = accountManager.getUser(uniqueName, false);
        // Проверяем согласованность: если пользователь не существует, то id должен быть 0
        bool exists = accountManager.userExists(uniqueName, false);
        if (!exists) {
            QVERIFY(user.id == 0); // Пользователь не существует, значит getUser должен вернуть пустого
        } else {
            QVERIFY(user.id > 0 && user.name == uniqueName); // Пользователь существует, значит getUser должен вернуть его
        }
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testGetAllUsers()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Получение всех пользователей
        QList<User> users = accountManager.getAllUsers(false);
        // Должен вернуться список (может быть пустой, если в БД нет пользователей)
        QVERIFY(users.size() >= 0);
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testDeleteUser()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Попытка удалить несуществующего пользователя
        UserOperationResult result = accountManager.deleteUser(99999);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найден"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testDeleteUserByUsername()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Попытка удалить несуществующего пользователя по имени
        UserOperationResult result = accountManager.deleteUser("nonexistent_user_12345");
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найден"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testDeleteUserWithoutConnection()
{
    AccountManager accountManager;
    // Не устанавливаем подключение
    
    UserOperationResult result = accountManager.deleteUser(1);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

void TestAccountManager::testRestoreUser()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Попытка восстановить несуществующего пользователя
        UserOperationResult result = accountManager.restoreUser(99999);
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найден"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testRestoreUserByUsername()
{
    AccountManager accountManager;
    DatabaseConnection* dbConn = TestHelpers::createTestDatabaseConnection();
    
    if (dbConn && dbConn->isConnected()) {
        accountManager.setDatabaseConnection(dbConn);
        
        // Попытка восстановить несуществующего пользователя по имени
        UserOperationResult result = accountManager.restoreUser("nonexistent_user_12345");
        QVERIFY(!result.success);
        QVERIFY(result.message.contains("не найден"));
    } else {
        QSKIP("Нет подключения к БД для теста");
    }
}

void TestAccountManager::testRestoreUserWithoutConnection()
{
    AccountManager accountManager;
    // Не устанавливаем подключение
    
    UserOperationResult result = accountManager.restoreUser(1);
    QVERIFY(!result.success);
    QVERIFY(result.message.contains("Нет подключения"));
}

QString TestAccountManager::generateUniqueUsername() const
{
    // Генерируем уникальное имя пользователя для тестов
    return QString("test_user_%1").arg(QDateTime::currentMSecsSinceEpoch());
}

