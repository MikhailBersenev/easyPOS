#include "easyposcore.h"
#include "RBAC/authmanager.h"
#include "settings/settingsmanager.h"
#include "RBAC/accountmanager.h"
#include "RBAC/rolemanager.h"
#include "sales/stockmanager.h"
#include "sales/salesmanager.h"
#include "production/productionmanager.h"
#include "shifts/shiftmanager.h"
#include "alerts/alertsmanager.h"
#include "alerts/alertkeys.h"
#include "logging/logmanager.h"
#include <QSqlQuery>

EasyPOSCore::EasyPOSCore()
    : databaseConnection(nullptr)
    , stockManager(nullptr)
    , productionManager(nullptr)
    , shiftManager(nullptr)
    , alertsManager(nullptr)
    , settingsManager(nullptr)
    , authManager(nullptr)
{
    qInfo() << "EasyPOSCore::EasyPOSCore()";
    createSettingsManager(this);
    m_productVersion = "26.02";
}

SettingsManager* EasyPOSCore::createSettingsManager(QObject *parent)
{
    if (settingsManager)
        return settingsManager;
    settingsManager = new SettingsManager(parent ? parent : this);
    qInfo() << "EasyPOSCore: SettingsManager created";
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
        alertsManager = new AlertsManager(this);
        alertsManager->setDatabaseConnection(databaseConnection);
        alertsManager->log(AlertCategory::System, AlertSignature::DbConnected,
                          QStringLiteral("Подключение к БД: %1").arg(authConfig.database));
        qInfo() << "EasyPOSCore: StockManager, ProductionManager, ShiftManager, AlertsManager created";
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
        qInfo() << "database connection is already initialized";
        delete databaseConnection;
        return;
    }
#endif
    databaseConnection = new DatabaseConnection();
    // Попытка подключения
    if (databaseConnection->connect(authConfig)) {
        qInfo() << "EasyPOSCore: DB connected," << databaseConnection->connectionName();
    } else {
        QSqlError error = databaseConnection->lastError();
        qWarning() << "EasyPOSCore: DB connect failed:" << error.text();
        // Приложение продолжит работу даже при ошибке подключения
    }
}

void EasyPOSCore::CloseDbConnection()
{
    if(!databaseConnection) {
        return;
    }
    if(databaseConnection->isConnected()) {
        qInfo() << "EasyPOSCore: closing DB connection";
        databaseConnection->disconnect();
    }
    delete databaseConnection;
    databaseConnection = nullptr;
}

AuthManager* EasyPOSCore::createAuthManager(QObject *parent)
{
    Q_UNUSED(parent);
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create AuthManager, no DB connection";
        return nullptr;
    }
    if (authManager)
        return authManager;
    authManager = new AuthManager(this);
    authManager->setDatabaseConnection(databaseConnection);
    qInfo() << "AuthManager создан через фабричный метод";
    return authManager;
}

AccountManager* EasyPOSCore::createAccountManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create AccountManager, no DB connection";
        return nullptr;
    }
    
    AccountManager* accountManager = new AccountManager(parent ? parent : this);
    accountManager->setDatabaseConnection(databaseConnection);
    
    qInfo() << "AccountManager создан через фабричный метод";
    return accountManager;
}

RoleManager* EasyPOSCore::createRoleManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create RoleManager, no DB connection";
        return nullptr;
    }
    
    RoleManager* roleManager = new RoleManager(parent ? parent : this);
    roleManager->setDatabaseConnection(databaseConnection);
    
    qInfo() << "RoleManager создан через фабричный метод";
    return roleManager;
}

StockManager* EasyPOSCore::createStockManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create StockManager, no DB connection";
        return nullptr;
    }
    
    StockManager* stockManager = new StockManager(parent ? parent : this);
    stockManager->setDatabaseConnection(databaseConnection);
    
    qInfo() << "StockManager создан через фабричный метод";
    return stockManager;
}

SalesManager* EasyPOSCore::createSalesManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create SalesManager, no DB connection";
        return nullptr;
    }
    
    SalesManager* salesManager = new SalesManager(parent ? parent : this);
    salesManager->setDatabaseConnection(databaseConnection);
    
    // Передаем StockManager в SalesManager
    if (stockManager) {
        salesManager->setStockManager(stockManager);
        qInfo() << "StockManager передан в SalesManager";
    }
    if (settingsManager)
        salesManager->setSettingsManager(settingsManager);

    qInfo() << "SalesManager создан через фабричный метод";
    return salesManager;
}

ShiftManager* EasyPOSCore::createShiftManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create ShiftManager, no DB connection";
        return nullptr;
    }
    if (shiftManager)
        return shiftManager;
    shiftManager = new ShiftManager(parent ? parent : this);
    shiftManager->setDatabaseConnection(databaseConnection);
    qInfo() << "ShiftManager создан через фабричный метод";
    return shiftManager;
}

AlertsManager* EasyPOSCore::createAlertsManager(QObject *parent)
{
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qWarning() << "EasyPOSCore: cannot create AlertsManager, no DB connection";
        return nullptr;
    }
    if (alertsManager)
        return alertsManager;
    alertsManager = new AlertsManager(parent ? parent : this);
    alertsManager->setDatabaseConnection(databaseConnection);
    qInfo() << "AlertsManager создан через фабричный метод";
    return alertsManager;
}

QString EasyPOSCore::getProductVersion()
{
    qInfo() << QString("getProductVersion %1").arg(m_productVersion);
    return m_productVersion;

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
            if (alertsManager)
                alertsManager->log(AlertCategory::Auth, AlertSignature::SessionExpired,
                                  QStringLiteral("Сессия завершена (нет токена): %1").arg(m_currentSession.username),
                                  m_currentSession.userId, getEmployeeIdByUserId(m_currentSession.userId));
            qInfo() << "EasyPOSCore::ensureSessionValid: no token, invalidating session";
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
        if (alertsManager && m_currentSession.userId > 0)
            alertsManager->log(AlertCategory::Auth, AlertSignature::SessionExpired,
                              QStringLiteral("Сессия истекла или токен недействителен: %1").arg(m_currentSession.username),
                              m_currentSession.userId, getEmployeeIdByUserId(m_currentSession.userId));
        qInfo() << "EasyPOSCore::ensureSessionValid: token invalid or expired, invalidating session";
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
    if (m_currentSession.userId > 0)
        qInfo() << "EasyPOSCore::logout: user=" << m_currentSession.username << "userId=" << m_currentSession.userId;
    if (alertsManager && m_currentSession.userId > 0) {
        qint64 empId = getEmployeeIdByUserId(m_currentSession.userId);
        alertsManager->log(AlertCategory::Auth, AlertSignature::Logout,
                          QStringLiteral("Выход: %1").arg(m_currentSession.username),
                          m_currentSession.userId, empId);
    }
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
