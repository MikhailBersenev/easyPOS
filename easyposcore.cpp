#include "easyposcore.h"
#include "RBAC/authmanager.h"
#include "RBAC/accountmanager.h"
#include "RBAC/rolemanager.h"
#include "sales/stockmanager.h"
#include "sales/salesmanager.h"
#include <QDebug>
#include <QSqlQuery>

EasyPOSCore::EasyPOSCore()
    : databaseConnection(nullptr)
    , stockManager(nullptr)
{
    qDebug() << "EasyPOSCore::EasyPOSCore()";
    //Создаем подключение к базе данных
    PostgreSQLAuth authConfig;
    // Настройка параметров подключения
    authConfig.host = "192.168.0.202"; //Хост
    authConfig.port = 5432; //Порт
    authConfig.database = "pos_bakery";  // имя базы данных
    authConfig.username = "postgres"; // Имя пользователя
    authConfig.password = "123456";   //Пароль
    authConfig.sslMode = "prefer";
    authConfig.connectTimeout = 10;

    this->CreateDbConnection(authConfig);
    
    // Создаем единственный экземпляр StockManager
    if (databaseConnection && databaseConnection->isConnected()) {
        stockManager = new StockManager(this);
        stockManager->setDatabaseConnection(databaseConnection);
        qDebug() << "StockManager создан в EasyPOSCore";
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
    if (!databaseConnection || !databaseConnection->isConnected()) {
        qDebug() << "Не удалось создать AuthManager: нет подключения к БД";
        return nullptr;
    }
    
    AuthManager* authManager = new AuthManager(parent ? parent : this);
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
