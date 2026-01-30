#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QString>

struct PostgreSQLAuth {
    QString host = "localhost";
    int port = 5432;
    QString database;
    QString username;
    QString password;
    QString sslMode = "prefer";  // disable, allow, prefer, require, verify-ca, verify-full
    int connectTimeout = 10;     // секунды
    
    // Проверка валидности данных
    bool isValid() const {
        return !host.isEmpty() && 
               port > 0 && port < 65536 && 
               !database.isEmpty() && 
               !username.isEmpty();
    }
    
    // Формирование строки подключения
    QString connectionString() const {
        return QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 sslmode=%6 connect_timeout=%7")
            .arg(host)
            .arg(port)
            .arg(database)
            .arg(username)
            .arg(password)
            .arg(sslMode)
            .arg(connectTimeout);
    }
};

#endif // STRUCTURES_H
