#ifndef ROLEMANAGER_H
#define ROLEMANAGER_H

#include <QObject>
#include <QString>
#include <QSqlError>
#include <QList>
#include "structures.h"
#include "../db/databaseconnection.h"

// Результат операции с ролью
struct RoleOperationResult {
    bool success;
    QString message;
    int roleId;  // ID созданной/обновленной роли
    QSqlError error;
    
    RoleOperationResult() : success(false), roleId(0) {}
};

class RoleManager : public QObject
{
    Q_OBJECT
public:
    explicit RoleManager(QObject *parent = nullptr);
    ~RoleManager();

    // Установка подключения к базе данных
    void setDatabaseConnection(DatabaseConnection *dbConnection);
    
    // === Управление ролями ===
    
    // Создание новой роли
    RoleOperationResult createRole(const QString &name, int level = 0);
    
    // Удаление роли (мягкое удаление - установка isdeleted = true)
    RoleOperationResult deleteRole(int roleId);
    RoleOperationResult deleteRole(const QString &roleName);
    
    // Восстановление удаленной роли
    RoleOperationResult restoreRole(int roleId);
    RoleOperationResult restoreRole(const QString &roleName);
    
    // Обновление роли
    RoleOperationResult updateRole(int roleId, const QString &name, int level);
    
    // Блокировка/разблокировка роли
    RoleOperationResult blockRole(int roleId, bool blocked = true);
    RoleOperationResult blockRole(const QString &roleName, bool blocked = true);
    
    // Получение роли
    Role getRole(int roleId, bool includeDeleted = false);
    Role getRole(const QString &roleName, bool includeDeleted = false);
    
    // Получение всех ролей
    QList<Role> getAllRoles(bool includeDeleted = false);
    
    // Проверка существования роли
    bool roleExists(int roleId, bool includeDeleted = false);
    bool roleExists(const QString &roleName, bool includeDeleted = false);

    // Получение последней ошибки
    QSqlError lastError() const;

signals:
    // Сигналы для уведомления о событиях
    void roleCreated(int roleId, const QString &roleName);
    void roleDeleted(int roleId, const QString &roleName);
    void roleRestored(int roleId, const QString &roleName);
    void roleUpdated(int roleId, const QString &roleName);
    void roleBlocked(int roleId, bool blocked);
    
    void operationFailed(const QString &message);

private:
    DatabaseConnection *m_dbConnection;
    QSqlError m_lastError;
    
    // Вспомогательные методы
    int getRoleIdByName(const QString &roleName, bool includeDeleted = false);
    
    // Вспомогательный метод для установки статуса удаления роли
    RoleOperationResult setRoleDeletedStatus(int roleId, bool isDeleted);
};

#endif // ROLEMANAGER_H
