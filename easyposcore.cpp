#include "easyposcore.h"
#include "RBAC/authmanager.h"
#include "settings/settingsmanager.h"
#include "RBAC/accountmanager.h"
#include "RBAC/rolemanager.h"
#include "sales/stockmanager.h"
#include "sales/salesmanager.h"
#include "production/productionmanager.h"
#include "shifts/shiftmanager.h"
#include "settings/settingsmanager.h"
#include <QDebug>
#include <QSqlQuery>

EasyPOSCore::EasyPOSCore()
    : databaseConnection(nullptr)
    , stockManager(nullptr)
    , productionManager(nullptr)
    , shiftManager(nullptr)
    , settingsManager(nullptr)
    , authManager(nullptr)
{
    qDebug() << "EasyPOSCore::EasyPOSCore()";
    createSettingsManager(this);
}

SettingsManager* EasyPOSCore::createSettingsManager(QObject *parent)
{
    if (settingsManager)
        return settingsManager;
    settingsManager = new SettingsManager(parent ? parent : this);
    qDebug() << "SettingsManager создан через фабричный метод";
    return settingsManager;
}

void EasyPOSCore::ensureDbConnection()
{
    if (!settingsManager)
        createSettingsManager(this);

    if (databaseConnection && databaseConnection->isConnected())
        return;  // Уже подключено

    PostgreSQLAuth authConfig = settingsManager->loadDatabaseSettings();
    if (authConfig.database.isEmpty())
        authConfig.database = QStringLiteral("pos_bakery");
    if (authConfig.username.isEmpty())
        authConfig.username = QStringLiteral("postgres");

    CreateDbConnection(authConfig);

    if (databaseConnection && databaseConnection->isConnected()) {
        stockManager = new StockManager(this);
        stockManager->setDatabaseConnection(databaseConnection);
        productionManager = new ProductionManager(this);
        productionManager->setDatabaseConnection(databaseConnection);
        productionManager->setStockManager(stockManager);
        shiftManager = new ShiftManager(this);
        shiftManager->setDatabaseConnection(databaseConnection);
        qDebug() << "StockManager, ProductionManager, ShiftManager созданы в EasyPOSCore";
    }
}

EasyPOSCore::~EasyPOSCore()
{
    // StockManager будет удален автоматически (parent = this)
    CloseDbConnection();
}


void EasyPOSCore::CreateDbConnection(PostgreSQLAuth authConfig)
{
#if 0
    if(databaseConnection) {
        qDebug() << "database connection is already initialized";
        delete databaseConnection;
        return;
    }
#endif
    databaseConnection = new DatabaseConnection();
    // Попытка подключения
    if (databaseConnection->connect(authConfig)) {
        qDebug() << "Успешное подключение к базе данных";
        qDebug() << "Имя подключения:" << databaseConnection->connectionName();
    } else {
        qDebug() << "Ошибка подключения к базе данных:";
        QSqlError error = databaseConnection->lastError();
        qDebug() << "Текст ошибки:" << error.text();
        qDebug() << "Тип ошибки:" << error.type();
        // Приложение продолжит работу даже при ошибке подключения
    }
}

void EasyPOSCore::CloseDbConnection()
{
    if(!databaseConnection) {
        qDebug() << "database connection is not initialized";
        return;
    }
    if(databaseConnection->isConnected()) {
        databaseConnection->disconnect();
    }
    delete databaseConnection;
    databaseConnection = nullptr;
}

AuthManager* EasyPOSCore::createAuthManager(QObject *parent)
{
    Q_UNUSED(parent);
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать AuthManager: нет подключения к БД";
        return nullptr;
    }
    if (authManager)
        return authManager;
    authManager = new AuthManager(this);
    authManager->setDatabaseConnection(databaseConnection);
    qDebug() << "AuthManager создан через фабричный метод";
    return authManager;
}

AccountManager* EasyPOSCore::createAccountManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать AccountManager: нет подключения к БД";
        return nullptr;
    }
    
    AccountManager* accountManager = new AccountManager(parent ? parent : this);
    accountManager->setDatabaseConnection(databaseConnection);
    
    qDebug() << "AccountManager создан через фабричный метод";
    return accountManager;
}

RoleManager* EasyPOSCore::createRoleManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать RoleManager: нет подключения к БД";
        return nullptr;
    }
    
    RoleManager* roleManager = new RoleManager(parent ? parent : this);
    roleManager->setDatabaseConnection(databaseConnection);
    
    qDebug() << "RoleManager создан через фабричный метод";
    return roleManager;
}

StockManager* EasyPOSCore::createStockManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать StockManager: нет подключения к БД";
        return nullptr;
    }
    
    StockManager* stockManager = new StockManager(parent ? parent : this);
    stockManager->setDatabaseConnection(databaseConnection);
    
    qDebug() << "StockManager создан через фабричный метод";
    return stockManager;
}

SalesManager* EasyPOSCore::createSalesManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать SalesManager: нет подключения к БД";
        return nullptr;
    }
    
    SalesManager* salesManager = new SalesManager(parent ? parent : this);
    salesManager->setDatabaseConnection(databaseConnection);
    
    // Передаем StockManager в SalesManager
    if (stockManager) {
        salesManager->setStockManager(stockManager);
        qDebug() << "StockManager передан в SalesManager";
    }
    
    qDebug() << "SalesManager создан через фабричный метод";
    return salesManager;
}

ShiftManager* EasyPOSCore::createShiftManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать ShiftManager: нет подключения к БД";
        return nullptr;
    }
    if (shiftManager)
        return shiftManager;
    shiftManager = new ShiftManager(parent ? parent : this);
    shiftManager->setDatabaseConnection(databaseConnection);
    qDebug() << "ShiftManager создан через фабричный метод";
    return shiftManager;
}

bool EasyPOSCore::isDatabaseConnected() const
{
    return databaseConnection && databaseConnection->isConnected();
}

qint64 EasyPOSCore::getEmployeeIdByUserId(qint64 userId) const
{
    if (!databaseConnection || !databaseConnection->isConnected() || userId <= 0)
        return 0;
    QSqlQuery q(databaseConnection->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id FROM employees WHERE userid = :uid AND (isdeleted IS NULL OR isdeleted = false) LIMIT 1"));
    q.bindValue(QStringLiteral(":uid"), userId);
    if (!q.exec() || !q.next())
        return 0;
    return q.value(0).toLongLong();
}

void EasyPOSCore::setCurrentSession(const UserSession &session)
{
    m_currentSession = session;
}

UserSession EasyPOSCore::getCurrentSession() const
{
    return m_currentSession;
}

bool EasyPOSCore::hasActiveSession() const
{
    return m_currentSession.userId > 0;
}

bool EasyPOSCore::ensureSessionValid()
{
    if (!settingsManager)
        return false;
    QString token = settingsManager->stringValue(SettingsKeys::SessionToken);
    if (token.isEmpty()) {
        if (m_currentSession.userId > 0) {
            m_currentSession = UserSession();
            emit sessionInvalidated();
        }
        return false;
    }
    AuthManager *am = createAuthManager();
    if (!am)
        return false;
    UserSession s = am->getSessionByToken(token);
    if (!s.isValid() || s.isExpired()) {
        settingsManager->remove(SettingsKeys::SessionToken);
        settingsManager->sync();
        m_currentSession = UserSession();
        emit sessionInvalidated();
        return false;
    }
    m_currentSession = s;
    am->updateSessionActivity(s.userId);
    return true;
}

void EasyPOSCore::logout()
{
    if (authManager && m_currentSession.userId > 0) {
        authManager->logout(m_currentSession.userId);
    }
    if (settingsManager) {
        settingsManager->remove(SettingsKeys::SessionToken);
        settingsManager->sync();
    }
    m_currentSession = UserSession();
    emit sessionInvalidated();
}
