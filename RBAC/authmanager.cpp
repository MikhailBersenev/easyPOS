#include "authmanager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QUuid>
#include <QDebug>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
    , m_dbConnection(nullptr)
{
}

AuthManager::~AuthManager()
{
    // Очистка сессий при уничтожении
    m_activeSessions.clear();
}

void AuthManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    m_dbConnection = dbConnection;
}

AuthResult AuthManager::authenticate(const QString &username, const QString &password)
{
    AuthResult result;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", 
                               "Нет подключения к базе данных", 
                               QSqlError::ConnectionError);
        result.error = m_lastError;
        return result;
    }
    
    if (username.isEmpty() || password.isEmpty()) {
        result.message = "Имя пользователя и пароль не могут быть пустыми";
        return result;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    qDebug() << "========================================";
    qDebug() << "ПОПЫТКА АВТОРИЗАЦИИ ПОЛЬЗОВАТЕЛЯ";
    qDebug() << "========================================";
    qDebug() << "Имя пользователя:" << username;
    qDebug() << "База данных:" << db.databaseName();
    qDebug() << "Хост:" << db.hostName();
    qDebug() << "Порт:" << db.port();
    
    // Проверка существования таблицы users
    QSqlQuery checkTableQuery(db);
    checkTableQuery.prepare("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_schema = 'public' AND table_name = 'users')");
    if (checkTableQuery.exec() && checkTableQuery.next()) {
        bool tableExists = checkTableQuery.value(0).toBool();
        qDebug() << "Таблица 'users' существует:" << (tableExists ? "ДА" : "НЕТ");
        if (!tableExists) {
            result.message = "Таблица 'users' не найдена в базе данных";
            qDebug() << "ОШИБКА:" << result.message;
            return result;
        }
    } else {
        qDebug() << "Предупреждение: не удалось проверить существование таблицы 'users'";
    }
    
    // Подсчет общего количества пользователей в таблице
    QSqlQuery countQuery(db);
    countQuery.prepare("SELECT COUNT(*) FROM users");
    if (countQuery.exec() && countQuery.next()) {
        int totalUsers = countQuery.value(0).toInt();
        qDebug() << "Всего пользователей в таблице:" << totalUsers;
    }
    
    // Подсчет активных (не удаленных) пользователей
    QSqlQuery activeCountQuery(db);
    activeCountQuery.prepare("SELECT COUNT(*) FROM users WHERE isdeleted = false");
    if (activeCountQuery.exec() && activeCountQuery.next()) {
        int activeUsers = activeCountQuery.value(0).toInt();
        qDebug() << "Активных пользователей (isdeleted = false):" << activeUsers;
    }
    
    // Поиск пользователя по имени
    query.prepare("SELECT id, name, password, roleid FROM users WHERE name = :username AND isdeleted = false");
    query.bindValue(":username", username);
    
    qDebug() << "SQL запрос:" << query.lastQuery();
    qDebug() << "Параметр :username =" << username;
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        result.message = "Ошибка при запросе к базе данных: " + m_lastError.text();
        result.error = m_lastError;
        qDebug() << "ОШИБКА выполнения SQL запроса:";
        qDebug() << "  Тип ошибки:" << m_lastError.type();
        qDebug() << "  Текст ошибки:" << m_lastError.text();
        qDebug() << "  Код ошибки:" << m_lastError.nativeErrorCode();
        qDebug() << "  Сообщение драйвера:" << m_lastError.driverText();
        return result;
    }
    
    // Проверка, есть ли пользователи с похожим именем (для диагностики)
    QSqlQuery similarQuery(db);
    similarQuery.prepare("SELECT id, name, isdeleted FROM users WHERE LOWER(name) = LOWER(:username)");
    similarQuery.bindValue(":username", username);
    if (similarQuery.exec()) {
        bool foundSimilar = false;
        while (similarQuery.next()) {
            if (!foundSimilar) {
                qDebug() << "Найдены пользователи с похожим именем (без учета регистра):";
                foundSimilar = true;
            }
            int similarId = similarQuery.value(0).toInt();
            QString similarName = similarQuery.value(1).toString();
            bool isDeleted = similarQuery.value(2).toBool();
            qDebug() << "  ID:" << similarId << "Имя:" << similarName << "Удален:" << (isDeleted ? "ДА" : "НЕТ");
        }
        if (!foundSimilar) {
            qDebug() << "Пользователи с похожим именем (без учета регистра) не найдены";
        }
    }
    
    if (!query.next()) {
        result.message = "Пользователь не найден";
        qDebug() << "========================================";
        qDebug() << "ПОЛЬЗОВАТЕЛЬ НЕ НАЙДЕН";
        qDebug() << "========================================";
        qDebug() << "Искали пользователя с именем:" << username;
        qDebug() << "Длина имени:" << username.length();
        qDebug() << "Имя в байтах:" << username.toUtf8().toHex();
        
        // Вывод списка всех пользователей для диагностики
        QSqlQuery allUsersQuery(db);
        allUsersQuery.prepare("SELECT id, name, isdeleted FROM users ORDER BY name");
        if (allUsersQuery.exec()) {
            qDebug() << "Список всех пользователей в базе данных:";
            int userCount = 0;
            while (allUsersQuery.next()) {
                userCount++;
                int userId = allUsersQuery.value(0).toInt();
                QString userName = allUsersQuery.value(1).toString();
                bool isDeleted = allUsersQuery.value(2).toBool();
                qDebug() << QString("  [%1] ID: %2, Имя: '%3', Удален: %4")
                            .arg(userCount)
                            .arg(userId)
                            .arg(userName)
                            .arg(isDeleted ? "ДА" : "НЕТ");
            }
            if (userCount == 0) {
                qDebug() << "  (таблица пуста)";
            }
        }
        
        qDebug() << "========================================";
        return result;
    }
    
    int userId = query.value(0).toInt();
    QString dbName = query.value(1).toString();
    QString dbPassword = query.value(2).toString();
    int roleId = query.value(3).toInt();
    
    qDebug() << "Пользователь найден!";
    qDebug() << "  ID:" << userId;
    qDebug() << "  Имя в БД:" << dbName;
    qDebug() << "  Роль ID:" << roleId;
    qDebug() << "  Длина пароля в БД:" << dbPassword.length();
    
    // Проверка пароля
    QString passwordHash = hashPassword(password);
    qDebug() << "Проверка пароля...";
    qDebug() << "  Хеш введенного пароля:" << passwordHash.left(20) << "...";
    qDebug() << "  Пароль в БД (первые 20 символов):" << dbPassword.left(20) << "...";
    
    if (passwordHash != dbPassword) {
        // Если пароль не совпадает с хешем, проверяем как обычный текст (для совместимости)
        if (dbPassword != password) {
            result.message = "Неверный пароль";
            qDebug() << "ОШИБКА: Пароль не совпадает (ни как хеш, ни как обычный текст)";
            qDebug() << "========================================";
            return result;
        } else {
            qDebug() << "Пароль совпадает как обычный текст (не хеширован в БД)";
        }
    } else {
        qDebug() << "Пароль совпадает (хеш)";
    }
    
    // Загрузка роли пользователя
    qDebug() << "Загрузка роли пользователя (ID:" << roleId << ")...";
    Role role = loadUserRole(roleId);
    if (role.id == 0) {
        result.message = "Роль пользователя не найдена";
        qDebug() << "ОШИБКА: Роль с ID" << roleId << "не найдена";
        qDebug() << "========================================";
        return result;
    }
    qDebug() << "Роль загружена:" << role.name;
    qDebug() << "Роль загружена, уровень доступа:" << role.level;
    
    // Создание сессии
    result.session = createSession(userId, username, role);
    m_activeSessions[userId] = result.session;
    
    // Сохранение сессии в БД для валидации по токену
    saveSessionToDb(result.session);
    
    result.success = true;
    result.message = "Авторизация успешна";
    
    qDebug() << "========================================";
    qDebug() << "АВТОРИЗАЦИЯ УСПЕШНА";
    qDebug() << "========================================";
    qDebug() << "ID пользователя:" << userId;
    qDebug() << "Имя пользователя:" << username;
    qDebug() << "Роль:" << role.name;
    qDebug() << "Токен сессии:" << result.session.sessionToken;
    qDebug() << "Время входа:" << result.session.loginDate.toString(Qt::ISODate) << result.session.loginTime.toString(Qt::ISODate);
    qDebug() << "========================================";
    
    emit userAuthenticated(userId, username);
    
    return result;
}

AuthResult AuthManager::authenticateByBadgeBarcode(const QString &badgeBarcode)
{
    AuthResult result;
    const QString code = badgeBarcode.trimmed();
    if (code.isEmpty()) {
        result.message = "Штрихкод бейджа не указан";
        return result;
    }

    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        result.message = "Нет подключения к базе данных";
        m_lastError = QSqlError("No database connection", result.message, QSqlError::ConnectionError);
        result.error = m_lastError;
        return result;
    }

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT e.userid, u.name, u.roleid FROM employees e "
        "JOIN users u ON u.id = e.userid AND (u.isdeleted IS NULL OR u.isdeleted = false) "
        "WHERE e.badgecode = :code AND (e.isdeleted IS NULL OR e.isdeleted = false)"));
    q.bindValue(QStringLiteral(":code"), code);
    if (!q.exec()) {
        m_lastError = q.lastError();
        result.message = "Ошибка при поиске сотрудника: " + m_lastError.text();
        result.error = m_lastError;
        return result;
    }
    if (!q.next()) {
        result.message = "Сотрудник с таким штрихкодом бейджа не найден или не привязан к пользователю";
        return result;
    }

    const int userId = q.value(0).toInt();
    const QString username = q.value(1).toString();
    const int roleId = q.value(2).toInt();
    if (userId <= 0 || username.isEmpty()) {
        result.message = "Некорректные данные пользователя для сотрудника";
        return result;
    }

    Role role = loadUserRole(roleId);
    if (role.id == 0) {
        result.message = "Роль пользователя не найдена";
        return result;
    }

    result.session = createSession(userId, username, role);
    m_activeSessions[userId] = result.session;
    if (!saveSessionToDb(result.session)) {
        result.message = "Ошибка сохранения сессии";
        return result;
    }

    result.success = true;
    result.message = "Вход по бейджу выполнен";
    emit userAuthenticated(userId, username);
    return result;
}

bool AuthManager::logout(int userId)
{
    if (m_activeSessions.contains(userId)) {
        removeSessionFromDb(userId);
        m_activeSessions.remove(userId);
        emit userLoggedOut(userId);
        return true;
    }
    return false;
}

bool AuthManager::hasPermission(int userId, const QString &permissionName)
{
    UserSession session = getSession(userId);
    return hasPermission(session, permissionName);
}

bool AuthManager::hasPermission(const UserSession &session, const QString &permissionName)
{
    if (!session.isValid()) {
        return false;
    }
    
    if (session.isExpired()) {
        emit sessionExpired(session.userId);
        return false;
    }
    
    bool hasPerm = session.role.hasPermission(permissionName);
    
    if (!hasPerm) {
        emit permissionDenied(session.userId, permissionName);
    }
    
    return hasPerm;
}

UserSession AuthManager::getSession(int userId) const
{
    if (m_activeSessions.contains(userId)) {
        UserSession session = m_activeSessions[userId];
        if (session.isExpired()) {
            return UserSession(); // Возвращаем невалидную сессию
        }
        return session;
    }
    return UserSession();
}

UserSession AuthManager::getSession(const QString &username) const
{
    for (const UserSession &session : m_activeSessions) {
        if (session.username == username && !session.isExpired()) {
            return session;
        }
    }
    return UserSession();
}

UserSession AuthManager::getSessionByToken(const QString &token) const
{
    if (token.isEmpty())
        return UserSession();

    // Сначала проверяем кэш
    for (const UserSession &session : m_activeSessions) {
        if (session.sessionToken == token && !session.isExpired()) {
            return session;
        }
    }

    // Загружаем из БД
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return UserSession();

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT userid, logindate, logintime, lastactivitydate, lastactivitytime FROM usersessions "
        "WHERE token = :token"));
    q.bindValue(QStringLiteral(":token"), token);
    if (!q.exec() || !q.next())
        return UserSession();

    int userId = q.value(0).toInt();
    QDate lastActivityDate = q.value(3).toDate();
    QTime lastActivityTime = q.value(4).toTime();
    int timeoutMinutes = 30;
    if (lastActivityDate.isValid() && lastActivityTime.isValid()) {
        if (QDateTime(lastActivityDate, lastActivityTime).secsTo(QDateTime::currentDateTime()) > timeoutMinutes * 60)
            return UserSession();  // Сессия истекла
    }

    QSqlQuery userQ(db);
    userQ.prepare(QStringLiteral("SELECT name, roleid FROM users WHERE id = :id"));
    userQ.bindValue(QStringLiteral(":id"), userId);
    if (!userQ.exec() || !userQ.next())
        return UserSession();

    QString username = userQ.value(0).toString();
    int roleId = userQ.value(1).toInt();
    Role role = loadUserRole(roleId);
    if (role.id == 0)
        return UserSession();

    UserSession session;
    session.userId = userId;
    session.username = username;
    session.role = role;
    session.loginDate = q.value(1).toDate();
    session.loginTime = q.value(2).toTime();
    session.lastActivityDate = lastActivityDate;
    session.lastActivityTime = lastActivityTime;
    session.sessionToken = token;
    // Добавляем в кэш для последующих вызовов getSession(userId)
    m_activeSessions[userId] = session;
    return session;
}

bool AuthManager::isTokenValid(const QString &token) const
{
    return getSessionByToken(token).isValid();
}

bool AuthManager::isSessionValid(int userId) const
{
    UserSession session = getSession(userId);
    return session.isValid() && !session.isExpired();
}

bool AuthManager::isSessionValid(const UserSession &session) const
{
    return session.isValid() && !session.isExpired();
}

void AuthManager::updateSessionActivity(int userId)
{
    if (m_activeSessions.contains(userId)) {
        m_activeSessions[userId].lastActivityDate = QDate::currentDate();
        m_activeSessions[userId].lastActivityTime = QTime::currentTime();
        if (m_dbConnection && m_dbConnection->isConnected()) {
            QSqlDatabase db = m_dbConnection->getDatabase();
            QSqlQuery q(db);
            q.prepare(QStringLiteral(
                "UPDATE usersessions SET lastactivitydate = :d, lastactivitytime = :t WHERE userid = :userid"));
            q.bindValue(QStringLiteral(":d"), QDate::currentDate());
            q.bindValue(QStringLiteral(":t"), QTime::currentTime());
            q.bindValue(QStringLiteral(":userid"), userId);
            q.exec();
        }
    }
}

Role AuthManager::getUserRole(int userId)
{
    return loadUserRole(userId);
}

QList<Permission> AuthManager::getRolePermissions(int roleId)
{
    return loadRolePermissions(roleId);
}

QSqlError AuthManager::lastError() const
{
    return m_lastError;
}

QString AuthManager::hashPassword(const QString &password) const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(password.toUtf8());
    return hash.result().toHex();
}

bool AuthManager::verifyPassword(const QString &password, const QString &hash) const
{
    QString passwordHash = hashPassword(password);
    return passwordHash == hash;
}

Role AuthManager::loadUserRole(int roleId) const
{
    Role role;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return role;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    // Загрузка роли
    query.prepare("SELECT id, name, level, isblocked, isdeleted FROM roles WHERE id = :roleId");
    query.bindValue(":roleId", roleId);
    
    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError();
        return role;
    }
    
    role.id = query.value(0).toInt();
    role.name = query.value(1).toString();
    role.level = query.value(2).toInt();
    role.isBlocked = query.value(3).toBool();
    role.isActive = !query.value(4).toBool();
    
    // В текущей структуре БД нет таблицы разрешений
    // Разрешения не загружаются
    
    return role;
}

QList<Permission> AuthManager::loadRolePermissions(int roleId)
{
    QList<Permission> permissions;
    
    if (!m_dbConnection || !m_dbConnection->isConnected()) {
        return permissions;
    }
    
    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery query(db);
    
    // Загрузка разрешений роли через связующую таблицу
    // Предполагается структура: role_permissions (role_id, permission_id)
    query.prepare(
        "SELECT p.id, p.name, p.description, p.is_active "
        "FROM permissions p "
        "INNER JOIN role_permissions rp ON p.id = rp.permission_id "
        "WHERE rp.role_id = :roleId AND p.is_active = true"
    );
    query.bindValue(":roleId", roleId);
    
    if (!query.exec()) {
        m_lastError = query.lastError();
        return permissions;
    }
    
    while (query.next()) {
        Permission perm;
        perm.id = query.value(0).toInt();
        perm.name = query.value(1).toString();
        perm.description = query.value(2).toString();
        perm.isActive = query.value(3).toBool();
        permissions.append(perm);
    }
    
    return permissions;
}

UserSession AuthManager::createSession(int userId, const QString &username, const Role &role)
{
    UserSession session;
    session.userId = userId;
    session.username = username;
    session.role = role;
    session.loginDate = QDate::currentDate();
    session.loginTime = QTime::currentTime();
    session.lastActivityDate = QDate::currentDate();
    session.lastActivityTime = QTime::currentTime();
    session.sessionToken = generateSessionToken();
    
    return session;
}

QString AuthManager::generateSessionToken() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool AuthManager::saveSessionToDb(const UserSession &session) const
{
    if (!m_dbConnection || !m_dbConnection->isConnected() || !session.isValid())
        return false;

    // Удаляем старую сессию пользователя (один пользователь — одна сессия)
    removeSessionFromDb(session.userId);

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO usersessions (userid, token, logindate, logintime, lastactivitydate, lastactivitytime) "
        "VALUES (:userid, :token, :logindate, :logintime, :lastactivitydate, :lastactivitytime)"));
    q.bindValue(QStringLiteral(":userid"), session.userId);
    q.bindValue(QStringLiteral(":token"), session.sessionToken);
    q.bindValue(QStringLiteral(":logindate"), session.loginDate);
    q.bindValue(QStringLiteral(":logintime"), session.loginTime);
    q.bindValue(QStringLiteral(":lastactivitydate"), session.lastActivityDate);
    q.bindValue(QStringLiteral(":lastactivitytime"), session.lastActivityTime);

    if (!q.exec()) {
        m_lastError = q.lastError();
        return false;
    }
    return true;
}

bool AuthManager::removeSessionFromDb(int userId) const
{
    if (!m_dbConnection || !m_dbConnection->isConnected())
        return false;

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM usersessions WHERE userid = :userid"));
    q.bindValue(QStringLiteral(":userid"), userId);
    return q.exec();
}

bool AuthManager::removeSessionFromDbByToken(const QString &token) const
{
    if (!m_dbConnection || !m_dbConnection->isConnected() || token.isEmpty())
        return false;

    QSqlDatabase db = m_dbConnection->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM usersessions WHERE token = :token"));
    q.bindValue(QStringLiteral(":token"), token);
    return q.exec();
}

