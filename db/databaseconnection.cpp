#include "databaseconnection.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QUuid>

DatabaseConnection::DatabaseConnection()
    : m_connectionName(generateConnectionName())
{
}

DatabaseConnection::~DatabaseConnection()
{
    disconnect();
}

bool DatabaseConnection::connect(const PostgreSQLAuth &auth)
{
    // Проверка валидности данных авторизации
    if (!auth.isValid()) {
        m_lastError = QSqlError("Invalid auth data", 
                               "Не все обязательные поля заполнены", 
                               QSqlError::ConnectionError);
        return false;
    }

    // Отключаемся, если уже подключены
    if (isConnected()) {
        disconnect();
    }

    // Добавляем драйвер PostgreSQL, если его еще нет
    if (!QSqlDatabase::drivers().contains("QPSQL")) {
        m_lastError = QSqlError("Driver not found", 
                               "Драйвер PostgreSQL (QPSQL) не найден. Установите libqt6-sql-psql", 
                               QSqlError::ConnectionError);
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
        m_database = QSqlDatabase(); // Очищаем невалидное подключение
        return false;
    }

    m_lastError = QSqlError(); // Очищаем ошибку при успешном подключении
    return true;
}

void DatabaseConnection::disconnect()
{
    if (m_database.isValid() && m_database.isOpen()) {
        m_database.close();
    }
    
    // Удаляем подключение из пула
    if (QSqlDatabase::contains(m_connectionName)) {
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
    // Генерируем уникальное имя подключения на основе UUID
    return QString("easyPOS_connection_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

