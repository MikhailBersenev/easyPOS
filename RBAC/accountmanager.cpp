#include "accountmanager.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QList>
#include <QVariant>

AccountManager::AccountManager(QObject *parent)
    : QObject{parent}
    , m_dbConnection{nullptr}
{
}

AccountManager::~AccountManager()
{
}

void AccountManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    m_dbConnection = dbConnection;
}

UserOperationResult AccountManager::registerUser(const QString &username, const QString &password, int roleId)
{
    UserOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    // Проверка входных данных
    if (username.isEmpty() || password.isEmpty()) {
        result.message = "Имя пользователя и пароль не могут быть пустыми";
        emit operationFailed(result.message);
        return result;
    }
    
    // Проверка существования пользователя
    if (userExists(username, true)) {  // Проверяем даже удаленных
        result.message = "Пользователь с таким именем уже существует";
        emit operationFailed(result.message);
        return result;
    }
    
    // Проверка существования роли
    if (!roleExists(roleId)) {
        result.message = QString("Роль с ID %1 не найдена").arg(roleId);
        emit operationFailed(result.message);
        return result;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    // Хеширование пароля
    QString passwordHash = hashPassword(password);
    
    qDebug() << "========================================";
    qDebug() << "РЕГИСТРАЦИЯ ПОЛЬЗОВАТЕЛЯ";
    qDebug() << "========================================";
    qDebug() << "Имя пользователя:" << username;
    qDebug() << "Роль ID:" << roleId;
    
    // Вставка нового пользователя
    query.prepare("INSERT INTO users (name, password, roleid, isdeleted) VALUES (:username, :password, :roleid, false) RETURNING id");
    query.bindValue(":username", username);
    query.bindValue(":password", passwordHash);
    query.bindValue(":roleid", roleId);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при создании пользователя: " + m_lastError.text();
        result.error = m_lastError;
        qDebug() << "ОШИБКА:" << result.message;
        emit operationFailed(result.message);
        return result;
    }
    
    if (query.next()) {
        result.userId = query.value(0).toInt();
        result.success = true;
        result.message = QString("Пользователь '%1' успешно зарегистрирован").arg(username);
        
        qDebug() << "Пользователь успешно зарегистрирован";
        qDebug() << "ID пользователя:" << result.userId;
        qDebug() << "========================================";
        
        emit userRegistered(result.userId, username);
    } else {
        result.message = "Не удалось получить ID созданного пользователя";
        emit operationFailed(result.message);
    }
    
    return result;
}

UserOperationResult AccountManager::registerUser(const QString &username, const QString &password, const QString &roleName)
{
    int roleId = getRoleIdByName(roleName);
    if (roleId == 0) {
        UserOperationResult result;
        result.message = QString("Роль '%1' не найдена").arg(roleName);
        emit operationFailed(result.message);
        return result;
    }
    
    return registerUser(username, password, roleId);
}

UserOperationResult AccountManager::deleteUser(int userId)
{
    UserOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    // Проверка существования пользователя
    if (!userExists(userId, true)) {
        result.message = "Пользователь не найден";
        emit operationFailed(result.message);
        return result;
    }
    
    qDebug() << "========================================";
    qDebug() << "УДАЛЕНИЕ ПОЛЬЗОВАТЕЛЯ";
    qDebug() << "========================================";
    qDebug() << "ID пользователя:" << userId;
    
    result = setUserDeletedStatus(userId, true);
    
    if (result.success) {
        User user = getUser(userId, true);
        qDebug() << "Пользователь удален:" << user.name;
        qDebug() << "========================================";
        emit userDeleted(userId, user.name);
    }
    
    return result;
}

UserOperationResult AccountManager::deleteUser(const QString &username)
{
    User user = getUser(username, true);
    if (user.id == 0) {
        UserOperationResult result;
        result.message = "Пользователь не найден";
        emit operationFailed(result.message);
        return result;
    }
    
    return deleteUser(user.id);
}

UserOperationResult AccountManager::restoreUser(int userId)
{
    UserOperationResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        emit operationFailed(result.message);
        return result;
    }
    
    // Проверка существования пользователя (включая удаленных)
    if (!userExists(userId, true)) {
        result.message = "Пользователь не найден";
        emit operationFailed(result.message);
        return result;
    }
    
    qDebug() << "========================================";
    qDebug() << "ВОССТАНОВЛЕНИЕ ПОЛЬЗОВАТЕЛЯ";
    qDebug() << "========================================";
    qDebug() << "ID пользователя:" << userId;
    
    result = setUserDeletedStatus(userId, false);
    
    if (result.success) {
        User user = getUser(userId, false);
        qDebug() << "Пользователь восстановлен:" << user.name;
        qDebug() << "========================================";
        emit userRestored(userId, user.name);
    }
    
    return result;
}

UserOperationResult AccountManager::restoreUser(const QString &username)
{
    User user = getUser(username, true);
    if (user.id == 0) {
        UserOperationResult result;
        result.message = "Пользователь не найден";
        emit operationFailed(result.message);
        return result;
    }
    
    return restoreUser(user.id);
}

bool AccountManager::userExists(const QString &username, bool includeDeleted)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return false;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT COUNT(*) FROM users WHERE name = :username";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool AccountManager::userExists(int userId, bool includeDeleted)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return false;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT COUNT(*) FROM users WHERE id = :id";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":id", userId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

User AccountManager::getUser(int userId, bool includeDeleted)
{
    User user;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return user;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT id, name, password, roleid, isdeleted FROM users WHERE id = :id";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":id", userId);
    
    if (query.exec() && query.next()) {
        user.id = query.value(0).toInt();
        user.name = query.value(1).toString();
        user.password = query.value(2).toString();
        
        // Загрузка роли
        int roleId = query.value(3).toInt();
        QSqlQuery roleQuery(db);
        roleQuery.prepare("SELECT id, name, level, isblocked, isdeleted FROM roles WHERE id = :roleid");
        roleQuery.bindValue(":roleid", roleId);
        if (roleQuery.exec() && roleQuery.next()) {
            user.role.id = roleQuery.value(0).toInt();
            user.role.name = roleQuery.value(1).toString();
            user.role.level = roleQuery.value(2).toInt();
            user.role.isBlocked = roleQuery.value(3).toBool();
            user.role.isActive = !roleQuery.value(4).toBool();
        }
        
        user.isDeleted = query.value(4).toBool();
    }
    
    return user;
}

User AccountManager::getUser(const QString &username, bool includeDeleted)
{
    User user;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return user;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT id, name, password, roleid, isdeleted FROM users WHERE name = :username";
    if (!includeDeleted) {
        sql += " AND isdeleted = false";
    }
    
    query.prepare(sql);
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        user.id = query.value(0).toInt();
        user.name = query.value(1).toString();
        user.password = query.value(2).toString();
        
        // Загрузка роли
        int roleId = query.value(3).toInt();
        QSqlQuery roleQuery(db);
        roleQuery.prepare("SELECT id, name, level, isblocked, isdeleted FROM roles WHERE id = :roleid");
        roleQuery.bindValue(":roleid", roleId);
        if (roleQuery.exec() && roleQuery.next()) {
            user.role.id = roleQuery.value(0).toInt();
            user.role.name = roleQuery.value(1).toString();
            user.role.level = roleQuery.value(2).toInt();
            user.role.isBlocked = roleQuery.value(3).toBool();
            user.role.isActive = !roleQuery.value(4).toBool();
        }
        
        user.isDeleted = query.value(4).toBool();
    }
    
    return user;
}

QList<User> AccountManager::getAllUsers(bool includeDeleted)
{
    QList<User> users;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return users;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    QString sql = "SELECT id, name, password, roleid, isdeleted FROM users";
    if (!includeDeleted) {
        sql += " WHERE isdeleted = false";
    }
    sql += " ORDER BY name";
    
    query.prepare(sql);
    
    if (query.exec()) {
        while (query.next()) {
            User user;
            user.id = query.value(0).toInt();
            user.name = query.value(1).toString();
            user.password = query.value(2).toString();
            
            // Загрузка роли
            int roleId = query.value(3).toInt();
            QSqlQuery roleQuery(db);
            roleQuery.prepare("SELECT id, name, level, isblocked, isdeleted FROM roles WHERE id = :roleid");
            roleQuery.bindValue(":roleid", roleId);
        if (roleQuery.exec() && roleQuery.next()) {
            user.role.id = roleQuery.value(0).toInt();
            user.role.name = roleQuery.value(1).toString();
            user.role.level = roleQuery.value(2).toInt();
            user.role.isBlocked = roleQuery.value(3).toBool();
            user.role.isActive = !roleQuery.value(4).toBool();
        }
        
        user.isDeleted = query.value(4).toBool();
            users.append(user);
        }
    }
    
    return users;
}

UserOperationResult AccountManager::updateUserRole(int userId, int roleId)
{
    UserOperationResult result;
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        result.error = m_lastError;
        return result;
    }
    if (!roleExists(roleId)) {
        result.message = tr("Роль не найдена");
        return result;
    }
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE users SET roleid = :roleid WHERE id = :id AND (isdeleted IS NULL OR isdeleted = false)"));
    q.bindValue(QStringLiteral(":roleid"), roleId);
    q.bindValue(QStringLiteral(":id"), userId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        result.message = tr("Ошибка обновления роли: %1").arg(m_lastError.text());
        result.error = m_lastError;
        return result;
    }
    result.success = true;
    result.userId = userId;
    result.message = tr("Роль обновлена");
    return result;
}

UserOperationResult AccountManager::updateUserPassword(int userId, const QString &newPassword)
{
    UserOperationResult result;
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        result.error = m_lastError;
        return result;
    }
    if (newPassword.isEmpty()) {
        result.success = true;
        result.userId = userId;
        return result;
    }
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("UPDATE users SET password = :pwd WHERE id = :id"));
    q.bindValue(QStringLiteral(":pwd"), hashPassword(newPassword));
    q.bindValue(QStringLiteral(":id"), userId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        result.message = tr("Ошибка смены пароля: %1").arg(m_lastError.text());
        result.error = m_lastError;
        return result;
    }
    result.success = true;
    result.userId = userId;
    result.message = tr("Пароль обновлён");
    return result;
}

QString AccountManager::getEmployeeBadgeCode(int userId) const
{
    if (!m_dbConnection || !m_dbConnection->isConnected() || userId <= 0)
        return QString();
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral("SELECT badgecode FROM employees WHERE userid = :uid AND (isdeleted IS NULL OR isdeleted = false) LIMIT 1"));
    q.bindValue(QStringLiteral(":uid"), userId);
    if (!q.exec() || !q.next())
        return QString();
    return q.value(0).toString();
}

UserOperationResult AccountManager::setEmployeeBadgeCode(int userId, const QString &badgeCode)
{
    UserOperationResult result;
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        result.error = m_lastError;
        return result;
    }
    QSqlQuery q(m_dbConnection->getDatabase());
    q.prepare(QStringLiteral(
        "UPDATE employees SET badgecode = :code, updatedate = CURRENT_DATE WHERE userid = :uid"));
    q.bindValue(QStringLiteral(":code"), badgeCode.trimmed().isEmpty() ? QVariant() : badgeCode.trimmed());
    q.bindValue(QStringLiteral(":uid"), userId);
    if (!q.exec()) {
        m_lastError = q.lastError();
        result.message = tr("Ошибка сохранения штрихкода бейджа: %1").arg(m_lastError.text());
        result.error = m_lastError;
        return result;
    }
    result.success = true;
    result.userId = userId;
    result.message = tr("Штрихкод бейджа сохранён");
    return result;
}

QSqlError AccountManager::lastError() const
{
    return m_lastError;
}

QString AccountManager::hashPassword(const QString &password) const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(password.toUtf8());
    return hash.result().toHex();
}

bool AccountManager::roleExists(int roleId)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return false;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT COUNT(*) FROM roles WHERE id = :roleid");
    query.bindValue(":roleid", roleId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

int AccountManager::getRoleIdByName(const QString &roleName)
{
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return 0;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    query.prepare("SELECT id FROM roles WHERE name = :name");
    query.bindValue(":name", roleName);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

UserOperationResult AccountManager::setUserDeletedStatus(int userId, bool isDeleted)
{
    UserOperationResult result;
    
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
    
    query.prepare("UPDATE users SET isdeleted = :isdeleted WHERE id = :id RETURNING id");
    query.bindValue(":isdeleted", isDeleted);
    query.bindValue(":id", userId);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при обновлении статуса пользователя: " + m_lastError.text();
        result.error = m_lastError;
        return result;
    }
    
    if (query.next()) {
        result.userId = query.value(0).toInt();
        result.success = true;
        result.message = isDeleted ? "Пользователь успешно удален" : "Пользователь успешно восстановлен";
    } else {
        result.message = "Пользователь не найден";
    }
    
    return result;
}
