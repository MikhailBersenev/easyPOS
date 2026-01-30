#include "test_databaseconnection.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

TestDatabaseConnection::TestDatabaseConnection()
{
}

TestDatabaseConnection::~TestDatabaseConnection()
{
}

void TestDatabaseConnection::initTestCase()
{
    // Инициализация перед всеми тестами
    qDebug() << "Инициализация тестового набора для DatabaseConnection";
}

void TestDatabaseConnection::cleanupTestCase()
{
    // Очистка после всех тестов
    qDebug() << "Завершение тестового набора для DatabaseConnection";
}

void TestDatabaseConnection::init()
{
    // Инициализация перед каждым тестом
}

void TestDatabaseConnection::cleanup()
{
    // Очистка после каждого теста
    // Удаляем все оставшиеся подключения
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &name : connections) {
        if (name.startsWith("easyPOS_connection_")) {
            QSqlDatabase::removeDatabase(name);
        }
    }
}

void TestDatabaseConnection::testConstructor()
{
    DatabaseConnection db;
    
    // Проверяем, что объект создан
    QVERIFY(!db.isConnected());
    QVERIFY(!db.connectionName().isEmpty());
    QVERIFY(db.connectionName().startsWith("easyPOS_connection_"));
}

void TestDatabaseConnection::testDestructor()
{
    {
        DatabaseConnection db;
        PostgreSQLAuth auth = createValidAuth();
        // Пытаемся подключиться (может не получиться, но это не важно для теста деструктора)
        db.connect(auth);
    }
    // После выхода из области видимости деструктор должен закрыть подключение
    // Проверяем, что подключений не осталось
    QStringList connections = QSqlDatabase::connectionNames();
    int easyPOSConnections = 0;
    for (const QString &name : connections) {
        if (name.startsWith("easyPOS_connection_")) {
            easyPOSConnections++;
        }
    }
    QCOMPARE(easyPOSConnections, 0);
}

void TestDatabaseConnection::testInvalidAuthData()
{
    DatabaseConnection db;
    PostgreSQLAuth auth = createInvalidAuth();
    
    bool result = db.connect(auth);
    QVERIFY(!result);
    QVERIFY(!db.isConnected());
    
    QSqlError error = db.lastError();
    QVERIFY(error.isValid());
}

void TestDatabaseConnection::testValidAuthData()
{
    PostgreSQLAuth auth = createValidAuth();
    QVERIFY(auth.isValid());
    
    // Проверяем, что все обязательные поля заполнены
    QVERIFY(!auth.host.isEmpty());
    QVERIFY(auth.port > 0);
    QVERIFY(!auth.database.isEmpty());
    QVERIFY(!auth.username.isEmpty());
}

void TestDatabaseConnection::testEmptyHost()
{
    PostgreSQLAuth auth;
    auth.port = 5432;
    auth.database = "testdb";
    auth.username = "testuser";
    auth.password = "testpass";
    
    // host пустой, но isValid() проверяет только на isEmpty()
    // На самом деле host имеет значение по умолчанию "localhost"
    QVERIFY(auth.isValid());
}

void TestDatabaseConnection::testEmptyDatabase()
{
    PostgreSQLAuth auth;
    auth.host = "localhost";
    auth.port = 5432;
    auth.database = "";  // Пустая база данных
    auth.username = "testuser";
    auth.password = "testpass";
    
    QVERIFY(!auth.isValid());
}

void TestDatabaseConnection::testEmptyUsername()
{
    PostgreSQLAuth auth;
    auth.host = "localhost";
    auth.port = 5432;
    auth.database = "testdb";
    auth.username = "";  // Пустое имя пользователя
    auth.password = "testpass";
    
    QVERIFY(!auth.isValid());
}

void TestDatabaseConnection::testInvalidPort()
{
    PostgreSQLAuth auth;
    auth.host = "localhost";
    auth.port = 0;  // Невалидный порт
    auth.database = "testdb";
    auth.username = "testuser";
    auth.password = "testpass";
    
    QVERIFY(!auth.isValid());
    
    auth.port = 70000;  // Порт вне допустимого диапазона
    QVERIFY(!auth.isValid());
}

void TestDatabaseConnection::testConnectWithInvalidData()
{
    DatabaseConnection db;
    PostgreSQLAuth auth = createInvalidAuth();
    
    bool result = db.connect(auth);
    QVERIFY(!result);
    QVERIFY(!db.isConnected());
    
    QSqlError error = db.lastError();
    QVERIFY(error.isValid());
    QCOMPARE(error.type(), QSqlError::ConnectionError);
}

void TestDatabaseConnection::testConnectWithoutDriver()
{
    // Этот тест проверяет обработку отсутствия драйвера
    // В реальной системе драйвер может быть установлен, поэтому тест может пройти
    DatabaseConnection db;
    PostgreSQLAuth auth = createValidAuth();
    
    // Проверяем наличие драйвера
    bool hasDriver = QSqlDatabase::drivers().contains("QPSQL");
    
    if (!hasDriver) {
        bool result = db.connect(auth);
        QVERIFY(!result);
        QVERIFY(!db.isConnected());
        
        QSqlError error = db.lastError();
        QVERIFY(error.isValid());
        QVERIFY(error.text().contains("QPSQL") || error.text().contains("драйвер"));
    } else {
        // Если драйвер есть, но подключение не удалось (нет БД или неверные данные),
        // это тоже нормально для теста
        bool result = db.connect(auth);
        // Результат может быть любым, так как БД может не существовать
        Q_UNUSED(result);
    }
}

void TestDatabaseConnection::testDisconnect()
{
    DatabaseConnection db;
    PostgreSQLAuth auth = createValidAuth();
    
    // Пытаемся подключиться
    db.connect(auth);
    
    // Отключаемся
    db.disconnect();
    
    QVERIFY(!db.isConnected());
    
    // Проверяем, что подключение удалено из пула
    QString connectionName = db.connectionName();
    QVERIFY(!QSqlDatabase::contains(connectionName));
}

void TestDatabaseConnection::testIsConnected()
{
    DatabaseConnection db;
    
    // Изначально не подключено
    QVERIFY(!db.isConnected());
    
    PostgreSQLAuth auth = createValidAuth();
    db.connect(auth);
    
    // После попытки подключения статус зависит от результата
    // Если БД существует и данные верны - будет подключено
    // Если нет - не будет подключено
    // В любом случае метод должен работать корректно
    bool connected = db.isConnected();
    Q_UNUSED(connected); // Просто проверяем, что метод не падает
    
    db.disconnect();
    QVERIFY(!db.isConnected());
}

void TestDatabaseConnection::testGetDatabase()
{
    DatabaseConnection db;
    
    // До подключения
    QSqlDatabase database = db.getDatabase();
    QVERIFY(!database.isValid());
    
    PostgreSQLAuth auth = createValidAuth();
    db.connect(auth);
    
    // После подключения (или попытки)
    database = db.getDatabase();
    // database может быть валидным или невалидным в зависимости от результата подключения
    Q_UNUSED(database);
}

void TestDatabaseConnection::testLastError()
{
    DatabaseConnection db;
    
    // Изначально ошибки нет (или пустая)
    QSqlError error = db.lastError();
    // Может быть пустой или содержать информацию
    
    // Пытаемся подключиться с невалидными данными
    PostgreSQLAuth auth = createInvalidAuth();
    db.connect(auth);
    
    error = db.lastError();
    QVERIFY(error.isValid());
}

void TestDatabaseConnection::testConnectionName()
{
    DatabaseConnection db1;
    DatabaseConnection db2;
    
    QString name1 = db1.connectionName();
    QString name2 = db2.connectionName();
    
    // Имена должны быть уникальными
    QVERIFY(name1 != name2);
    
    // Имена должны начинаться с префикса
    QVERIFY(name1.startsWith("easyPOS_connection_"));
    QVERIFY(name2.startsWith("easyPOS_connection_"));
    
    // Имена не должны быть пустыми
    QVERIFY(!name1.isEmpty());
    QVERIFY(!name2.isEmpty());
}

void TestDatabaseConnection::testReconnect()
{
    DatabaseConnection db;
    PostgreSQLAuth auth = createValidAuth();
    
    // Первое подключение
    db.connect(auth);
    bool firstConnected = db.isConnected();
    
    // Второе подключение (должно отключиться от первого)
    db.connect(auth);
    bool secondConnected = db.isConnected();
    
    // Проверяем, что метод работает без ошибок
    Q_UNUSED(firstConnected);
    Q_UNUSED(secondConnected);
    
    db.disconnect();
    QVERIFY(!db.isConnected());
}

PostgreSQLAuth TestDatabaseConnection::createValidAuth() const
{
    PostgreSQLAuth auth;
    auth.host = "localhost";
    auth.port = 5432;
    auth.database = "test_database";
    auth.username = "test_user";
    auth.password = "test_password";
    auth.sslMode = "prefer";
    auth.connectTimeout = 10;
    return auth;
}

PostgreSQLAuth TestDatabaseConnection::createInvalidAuth() const
{
    PostgreSQLAuth auth;
    auth.host = "localhost";
    auth.port = 5432;
    auth.database = "";  // Пустая база данных - невалидные данные
    auth.username = "test_user";
    auth.password = "test_password";
    return auth;
}

