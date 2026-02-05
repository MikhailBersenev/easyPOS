#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QObject>
#include <QString>
#include <QSqlError>
#include <QList>
#include "../db/databaseconnection.h"
#include "structures.h"

// Результат операции с пользователем
struct UserOperationResult {
    bool success;
    QString message;
    int userId;  // ID созданного/обновленного пользователя
    QSqlError error;
    
    UserOperationResult() : success(false), userId(0) {}
};

class AccountManager : public QObject
{
    Q_OBJECT
public:
    explicit AccountManager(QObject *parent = nullptr);
    ~AccountManager();

    // Установка подключения к базе данных
    void setDatabaseConnection(DatabaseConnection *dbConnection);
    
    // Регистрация нового пользователя
    UserOperationResult registerUser(const QString &username, const QString &password, int roleId);
    UserOperationResult registerUser(const QString &username, const QString &password, const QString &roleName);
    
    // Удаление пользователя (мягкое удаление - установка isdeleted = true)
    UserOperationResult deleteUser(int userId);
    UserOperationResult deleteUser(const QString &username);
    
    // Восстановление удаленного пользователя
    UserOperationResult restoreUser(int userId);
    UserOperationResult restoreUser(const QString &username);
    
    // Проверка существования пользователя
    bool userExists(const QString &username, bool includeDeleted = false);
    bool userExists(int userId, bool includeDeleted = false);
    
    // Получение информации о пользователе
    User getUser(int userId, bool includeDeleted = false);
    User getUser(const QString &username, bool includeDeleted = false);
    
    // Получение всех пользователей
    QList<User> getAllUsers(bool includeDeleted = false);

    // Редактирование пользователя
    UserOperationResult updateUserRole(int userId, int roleId);
    UserOperationResult updateUserPassword(int userId, const QString &newPassword);
    /** Штрихкод бейджа сотрудника, привязанного к пользователю (из employees.badgecode) */
    QString getEmployeeBadgeCode(int userId) const;
    UserOperationResult setEmployeeBadgeCode(int userId, const QString &badgeCode);

    // Получение последней ошибки
    QSqlError lastError() const;

signals:
    // Сигналы для уведомления о событиях
    void userRegistered(int userId, const QString &username);
    void userDeleted(int userId, const QString &username);
    void userRestored(int userId, const QString &username);
    void operationFailed(const QString &message);

private:
    DatabaseConnection *m_dbConnection;
    QSqlError m_lastError;
    
    // Хеширование пароля
    QString hashPassword(const QString &password) const;
    
    // Проверка существования роли
    bool roleExists(int roleId);
    int getRoleIdByName(const QString &roleName);
    
    // Вспомогательный метод для установки isdeleted
    UserOperationResult setUserDeletedStatus(int userId, bool isDeleted);
};

#endif // ACCOUNTMANAGER_H
