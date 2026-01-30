#ifndef RBAC_STRUCTURES_H
#define RBAC_STRUCTURES_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QSqlError>

// Разрешение (Permission)
struct Permission {
    int id;
    QString name;           // Например: "products.create", "sales.view"
    QString description;    // Описание разрешения
    bool isActive;
    
    Permission() : id(0), isActive(true) {}
};

// Роль (соответствует таблице roles в БД)
struct Role {
    int id;
    QString name;           // Например: "admin", "cashier", "manager"
    int level;              // Уровень доступа (0 - самый секретный)
    bool isBlocked;         // Флаг блокировки
    bool isActive;          // Противоположно isdeleted
    
    Role() : id(0), level(0), isBlocked(false), isActive(true) {}
    
    // Проверка наличия разрешения (для обратной совместимости, но разрешения не используются)
    bool hasPermission(const QString &permissionName) const {
        // В текущей структуре БД нет таблицы разрешений
        // Можно реализовать проверку по уровню доступа
        Q_UNUSED(permissionName);
        return isActive && !isBlocked;
    }
};

// Сессия пользователя
struct UserSession {
    int userId;
    QString username;
    Role role;
    QDateTime loginTime;
    QDateTime lastActivity;
    QString sessionToken;   // Токен сессии для безопасности
    
    UserSession() : userId(0) {}
    
    bool isValid() const {
        return userId > 0 && !username.isEmpty();
    }
    
    bool isExpired(int timeoutMinutes = 30) const {
        if (!lastActivity.isValid()) {
            return true;
        }
        return lastActivity.secsTo(QDateTime::currentDateTime()) > (timeoutMinutes * 60);
    }
};

// Результат авторизации
struct AuthResult {
    bool success;
    QString message;
    UserSession session;
    QSqlError error;        // Ошибка базы данных, если была
    
    AuthResult() : success(false) {}
};

// Роль пользователя (для совместимости с User)
struct UserRole {
    int id;
    QString name;
    int level;
    bool isBlocked;
    bool isActive;
    bool isDeleted;
    
    UserRole() : id(0), level(0), isBlocked(false), isActive(true), isDeleted(false) {}
};

// Пользователь
struct User {
    int id;
    QString name;
    QString password;
    UserRole role;
    bool isDeleted;
    
    User() : id(0), isDeleted(false) {}
};

#endif // RBAC_STRUCTURES_H

