#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include "structures.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>

class DatabaseConnection
{
public:
    DatabaseConnection();
    ~DatabaseConnection();

    // Подключение к базе данных
    bool connect(const PostgreSQLAuth &auth);
    
    // Отключение от базы данных
    void disconnect();
    
    // Проверка подключения
    bool isConnected() const;
    
    // Получить объект базы данных
    QSqlDatabase getDatabase() const;
    
    // Получить последнюю ошибку
    QSqlError lastError() const;
    
    // Получить имя подключения
    QString connectionName() const;

private:
    QString m_connectionName;
    QSqlDatabase m_database;
    QSqlError m_lastError;
    
    // Генерация уникального имени подключения
    QString generateConnectionName() const;
};

#endif // DATABASECONNECTION_H

