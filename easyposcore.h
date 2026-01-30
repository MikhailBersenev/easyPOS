#ifndef EASYPOSCORE_H
#define EASYPOSCORE_H

#include <QObject>
#include "db/databaseconnection.h"
#include "RBAC/structures.h"

// Предварительные объявления
class AuthManager;
class AccountManager;
class RoleManager;
class StockManager;
class SalesManager;

class EasyPOSCore : public QObject
{
    Q_OBJECT
public:
    explicit EasyPOSCore();
    ~EasyPOSCore();
    
    // Получение подключения к БД
    DatabaseConnection* getDatabaseConnection() const { return databaseConnection; }
    
    // Фабричные методы для создания объектов
    AuthManager* createAuthManager(QObject *parent = nullptr);
    AccountManager* createAccountManager(QObject *parent = nullptr);
    RoleManager* createRoleManager(QObject *parent = nullptr);
    StockManager* createStockManager(QObject *parent = nullptr);
    SalesManager* createSalesManager(QObject *parent = nullptr);
    
    // Получение единственного экземпляра StockManager
    StockManager* getStockManager() const { return stockManager; }
    
    // Проверка подключения к БД
    bool isDatabaseConnected() const;

    // userid (users.id) -> employees.id для кассы
    qint64 getEmployeeIdByUserId(qint64 userId) const;
    
    // Управление сессией пользователя
    void setCurrentSession(const UserSession &session);
    UserSession getCurrentSession() const;
    bool hasActiveSession() const;

private:
    DatabaseConnection* databaseConnection;
    StockManager* stockManager;
    UserSession m_currentSession;
    
    void CreateDbConnection(PostgreSQLAuth authConfig);
    void CloseDbConnection();
};

#endif // EASYPOSCORE_H
