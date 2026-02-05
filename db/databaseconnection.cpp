#include "databaseconnection.h"
#include "../logging/logmanager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QUuid>

DatabaseConnection::DatabaseConnection()
    : m_connectionName(generateConnectionName())
{
    qDebug() << "DatabaseConnection::DatabaseConnection" << "name=" << m_connectionName;
}

DatabaseConnection::~DatabaseConnection()
{
    qDebug() << "DatabaseConnection::~DatabaseConnection" << m_connectionName;
    disconnect();
}

bool DatabaseConnection::connect(const PostgreSQLAuth &auth)
{
    qDebug() << "DatabaseConnection::connect" << "host=" << auth.host << "db=" << auth.database;
    // Проверка валидности данных авторизации
    if (!auth.isValid()) {
        qWarning() << "DatabaseConnection::connect: invalid auth data";
        m_lastError = QSqlError("Invalid auth data", 
                               "Не все обязательные поля заполнены", 
                               QSqlError::ConnectionError);
        qDebug() << "DatabaseConnection::connect: branch invalid_auth";
        return false;
    }

    // Отключаемся, если уже подключены
    if (isConnected()) {
        qDebug() << "DatabaseConnection::connect: branch already_connected, disconnecting";
        disconnect();
    }

    // Добавляем драйвер PostgreSQL, если его еще нет
    if (!QSqlDatabase::drivers().contains("QPSQL")) {
        qCritical() << "DatabaseConnection::connect: PostgreSQL driver (QPSQL) not found";
        m_lastError = QSqlError("Driver not found", 
                               "Драйвер PostgreSQL (QPSQL) не найден. Установите libqt6-sql-psql", 
                               QSqlError::ConnectionError);
        qDebug() << "DatabaseConnection::connect: branch driver_not_found";
        return false;
    }

    // Создаем подключение
    m_database = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
    
    // Устанавливаем параметры подключения
    m_database.setHostName(auth.host);
    m_database.setPort(auth.port);
    m_database.setDatabaseName(auth.database);
    m_database.setUserName(auth.username);
    m_database.setPassword(auth.password);
    
    // Дополнительные параметры через setConnectOptions
    QString connectOptions = QString("sslmode=%1;connect_timeout=%2")
        .arg(auth.sslMode)
        .arg(auth.connectTimeout);
    m_database.setConnectOptions(connectOptions);

    // Пытаемся открыть подключение
    if (!m_database.open()) {
        m_lastError = m_database.lastError();
        qWarning() << "DatabaseConnection::connect: open failed" << m_lastError.text() << "host=" << auth.host << "db=" << auth.database;
        m_database = QSqlDatabase(); // Очищаем невалидное подключение
        qDebug() << "DatabaseConnection::connect: branch open_failed";
        return false;
    }

    m_lastError = QSqlError(); // Очищаем ошибку при успешном подключении
    qDebug() << "DatabaseConnection::connect: branch success";
    qInfo() << "DatabaseConnection::connect: connected" << m_connectionName << "host=" << auth.host << "db=" << auth.database;
    return true;
}

void DatabaseConnection::disconnect()
{
    qDebug() << "DatabaseConnection::disconnect" << m_connectionName;
    if (m_database.isValid() && m_database.isOpen()) {
        qInfo() << "DatabaseConnection::disconnect:" << m_connectionName;
        m_database.close();
    }
    
    // Удаляем подключение из пула
    if (QSqlDatabase::contains(m_connectionName)) {
        qDebug() << "DatabaseConnection::disconnect: branch removeDatabase";
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    
    m_database = QSqlDatabase();
}

bool DatabaseConnection::isConnected() const
{
    return m_database.isValid() && m_database.isOpen();
}

QSqlDatabase DatabaseConnection::getDatabase() const
{
    return m_database;
}

QSqlError DatabaseConnection::lastError() const
{
    return m_lastError.isValid() ? m_lastError : m_database.lastError();
}

QString DatabaseConnection::connectionName() const
{
    return m_connectionName;
}

QString DatabaseConnection::generateConnectionName() const
{
    return QString("easyPOS_connection_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

