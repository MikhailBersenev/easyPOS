#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include "structures.h"
#include "../db/databaseconnection.h"
#include "structures.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QCryptographicHash>
#include <QHash>

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject *parent = nullptr);
    ~AuthManager();

    // Установка подключения к базе данных
    void setDatabaseConnection(DatabaseConnection *dbConnection);
    
    // Авторизация пользователя
    AuthResult authenticate(const QString &username, const QString &password);
    
    // Выход из системы
    bool logout(int userId);
    
    // Проверка прав доступа
    bool hasPermission(int userId, const QString &permissionName);
    bool hasPermission(const UserSession &session, const QString &permissionName);
    
    // Получение сессии пользователя
    UserSession getSession(int userId) const;
    UserSession getSession(const QString &username) const;
    
    // Проверка валидности сессии
    bool isSessionValid(int userId) const;
    bool isSessionValid(const UserSession &session) const;
    
    // Обновление активности сессии
    void updateSessionActivity(int userId);
    
    // Получение роли пользователя
    Role getUserRole(int userId);
    
    // Получение всех разрешений роли
    QList<Permission> getRolePermissions(int roleId);
    
    // Получение последней ошибки
    QSqlError lastError() const;

signals:
    // Сигналы для уведомления о событиях авторизации
    void userAuthenticated(int userId, const QString &username);
    void userLoggedOut(int userId);
    void sessionExpired(int userId);
    void permissionDenied(int userId, const QString &permission);

private:
    DatabaseConnection *m_dbConnection;
    QHash<int, UserSession> m_activeSessions;  // Кэш активных сессий
    QSqlError m_lastError;
    
    // Хеширование пароля
    QString hashPassword(const QString &password) const;
    
    // Проверка пароля
    bool verifyPassword(const QString &password, const QString &hash) const;
    
    // Загрузка роли пользователя из БД
    Role loadUserRole(int userId);
    
    // Загрузка разрешений роли из БД
    QList<Permission> loadRolePermissions(int roleId);
    
    // Создание сессии
    UserSession createSession(int userId, const QString &username, const Role &role);
    
    // Генерация токена сессии
    QString generateSessionToken() const;
};

#endif // AUTHMANAGER_H

