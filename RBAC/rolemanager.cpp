#include "rolemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>

RoleManager::RoleManager(QObject *parent)
    : QObject(parent)
    , m_dbConnection(nullptr)
{
}

RoleManager::~RoleManager()
{
}

void RoleManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    m_dbConnection = dbConnection;
}

// === Управление ролями ===

RoleOperationResult RoleManager::createRole(const QString &name, int level)
{
    RoleOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (name.isEmpty()) {
        result.message = "Имя роли не может быть пустым";
        emit operationFailed(result.message);
        return result;
    }
    
    // Проверка существования роли
    if (roleExists(name, true)) {
        result.message = "Роль с таким именем уже существует";
        emit operationFailed(result.message);
        return result;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    qDebug() << "========================================";
    qDebug() << "СОЗДАНИЕ РОЛИ";
    qDebug() << "========================================";
    qDebug() << "Имя роли:" << name;
    qDebug() << "Уровень доступа:" << level;
    
    query.prepare("INSERT INTO roles (name, level, isblocked, isdeleted) VALUES (:name, :level, false, false) RETURNING id");
    query.bindValue(":name", name);
    query.bindValue(":level", level);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при создании роли: " + m_lastError.text();
        result.error = m_lastError;
        qDebug() << "ОШИБКА:" << result.message;
        emit operationFailed(result.message);
        return result;
    }
    
    if (query.next()) {
        result.roleId = query.value(0).toInt();
        result.success = true;
        result.message = QString("Роль '%1' успешно создана").arg(name);
        
        qDebug() << "Роль успешно создана";
        qDebug() << "ID роли:" << result.roleId;
        qDebug() << "========================================";
        
        emit roleCreated(result.roleId, name);
    } else {
        result.message = "Не удалось получить ID созданной роли";
        emit operationFailed(result.message);
    }
    
    return result;
}

RoleOperationResult RoleManager::deleteRole(int roleId)
{
    RoleOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (!roleExists(roleId, true)) {
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    qDebug() << "========================================";
    qDebug() << "УДАЛЕНИЕ РОЛИ";
    qDebug() << "========================================";
    qDebug() << "ID роли:" << roleId;
    
    result = setRoleDeletedStatus(roleId, true);
    
    if (result.success) {
        Role role = getRole(roleId, true);
        qDebug() << "Роль удалена:" << role.name;
        qDebug() << "========================================";
        emit roleDeleted(roleId, role.name);
    }
    
    return result;
}

RoleOperationResult RoleManager::deleteRole(const QString &roleName)
{
    int roleId = getRoleIdByName(roleName, true);
    if (roleId == 0) {
        RoleOperationResult result;
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    return deleteRole(roleId);
}

RoleOperationResult RoleManager::restoreRole(int roleId)
{
    RoleOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (!roleExists(roleId, true)) {
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    qDebug() << "========================================";
    qDebug() << "ВОССТАНОВЛЕНИЕ РОЛИ";
    qDebug() << "========================================";
    qDebug() << "ID роли:" << roleId;
    
    result = setRoleDeletedStatus(roleId, false);
    
    if (result.success) {
        Role role = getRole(roleId, false);
        qDebug() << "Роль восстановлена:" << role.name;
        qDebug() << "========================================";
        emit roleRestored(roleId, role.name);
    }
    
    return result;
}

RoleOperationResult RoleManager::restoreRole(const QString &roleName)
{
    int roleId = getRoleIdByName(roleName, true);
    if (roleId == 0) {
        RoleOperationResult result;
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    return restoreRole(roleId);
}

RoleOperationResult RoleManager::updateRole(int roleId, const QString &name, int level)
{
    RoleOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (name.isEmpty()) {
        result.message = "Имя роли не может быть пустым";
        emit operationFailed(result.message);
        return result;
    }
    
    if (!roleExists(roleId, true)) {
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE roles SET name = :name, level = :level WHERE id = :id RETURNING id");
    query.bindValue(":name", name);
    query.bindValue(":level", level);
    query.bindValue(":id", roleId);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при обновлении роли: " + m_lastError.text();
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (query.next()) {
        result.roleId = query.value(0).toInt();
        result.success = true;
        result.message = QString("Роль '%1' успешно обновлена").arg(name);
        emit roleUpdated(roleId, name);
    } else {
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
    }
    
    return result;
}

RoleOperationResult RoleManager::blockRole(int roleId, bool blocked)
{
    RoleOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (!roleExists(roleId, true)) {
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE roles SET isblocked = :isblocked WHERE id = :id RETURNING id");
    query.bindValue(":isblocked", blocked);
    query.bindValue(":id", roleId);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при изменении статуса блокировки роли: " + m_lastError.text();
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    if (query.next()) {
        result.roleId = query.value(0).toInt();
        result.success = true;
        result.message = blocked ? "Роль заблокирована" : "Роль разблокирована";
        emit roleBlocked(roleId, blocked);
    } else {
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
    }
    
    return result;
}

RoleOperationResult RoleManager::blockRole(const QString &roleName, bool blocked)
{
    int roleId = getRoleIdByName(roleName, true);
    if (roleId == 0) {
        RoleOperationResult result;
        result.message = "Роль не найдена";
        emit operationFailed(result.message);
        return result;
    }
    
    return blockRole(roleId, blocked);
}

Role RoleManager::getRole(int roleId, bool includeDeleted)
{
    Role role;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return role;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT id, name, level, isblocked, isdeleted FROM roles WHERE id = :id";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":id", roleId);
    
    if (query.exec() && query.next()) {
        role.id = query.value(0).toInt();
        role.name = query.value(1).toString();
        role.level = query.value(2).toInt();
        role.isBlocked = query.value(3).toBool();
        role.isActive = !query.value(4).toBool();
    }
    
    return role;
}

Role RoleManager::getRole(const QString &roleName, bool includeDeleted)
{
    int roleId = getRoleIdByName(roleName, includeDeleted);
    if (roleId == 0) {
        return Role();
    }
    
    return getRole(roleId, includeDeleted);
}

QList<Role> RoleManager::getAllRoles(bool includeDeleted)
{
    QList<Role> roles;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return roles;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT id, name, level, isblocked, isdeleted FROM roles";
    if (!includeDeleted) {
        sql += " WHERE isdeleted = false";
    }
    sql += " ORDER BY name";
    
    query.prepare(sql);
    
    if (query.exec()) {
        while (query.next()) {
            Role role;
            role.id = query.value(0).toInt();
            role.name = query.value(1).toString();
            role.level = query.value(2).toInt();
            role.isBlocked = query.value(3).toBool();
            role.isActive = !query.value(4).toBool();
            
            roles.append(role);
        }
    }
    
    return roles;
}

bool RoleManager::roleExists(int roleId, bool includeDeleted)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return false;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT COUNT(*) FROM roles WHERE id = :id";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":id", roleId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool RoleManager::roleExists(const QString &roleName, bool includeDeleted)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return false;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT COUNT(*) FROM roles WHERE name = :name";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":name", roleName);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

QSqlError RoleManager::lastError() const
{
    return m_lastError;
}

// === Вспомогательные методы ===

int RoleManager::getRoleIdByName(const QString &roleName, bool includeDeleted)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return 0;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT id FROM roles WHERE name = :name";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":name", roleName);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

RoleOperationResult RoleManager::setRoleDeletedStatus(int roleId, bool isDeleted)
{
    RoleOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        return result;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    query.prepare("UPDATE roles SET isdeleted = :isdeleted WHERE id = :id RETURNING id");
    query.bindValue(":isdeleted", isDeleted);
    query.bindValue(":id", roleId);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при обновлении статуса роли: " + m_lastError.text();
        result.error = m_lastError;
        return result;
    }
    
    if (query.next()) {
        result.roleId = query.value(0).toInt();
        result.success = true;
        result.message = isDeleted ? "Роль успешно удалена" : "Роль успешно восстановлена";
    } else {
        result.message = "Роль не найдена";
    }
    
    return result;
}
