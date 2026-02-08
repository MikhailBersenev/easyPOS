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
class ProductionManager;
class SettingsManager;
class ShiftManager;
class AlertsManager;

class EasyPOSCore : public QObject
{
    Q_OBJECT
public:
    explicit EasyPOSCore();
    ~EasyPOSCore();
    
    // Получение подключения к БД
    DatabaseConnection* getDatabaseConnection() const { return databaseConnection; }
    
    // Фабричные методы для создания объектов
    SettingsManager* createSettingsManager(QObject *parent = nullptr);
    AuthManager* createAuthManager(QObject *parent = nullptr);
    AccountManager* createAccountManager(QObject *parent = nullptr);
    RoleManager* createRoleManager(QObject *parent = nullptr);
    StockManager* createStockManager(QObject *parent = nullptr);
    SalesManager* createSalesManager(QObject *parent = nullptr);
    ShiftManager* createShiftManager(QObject *parent = nullptr);
    AlertsManager* createAlertsManager(QObject *parent = nullptr);

    // Получение менеджера настроек (создаётся при первом вызове createSettingsManager)
    SettingsManager* getSettingsManager() const { return settingsManager; }
    
    // Получение единственного экземпляра StockManager
    StockManager* getStockManager() const { return stockManager; }

    // Получение единственного экземпляра ProductionManager
    ProductionManager* getProductionManager() const { return productionManager; }
    ShiftManager* getShiftManager() const { return shiftManager; }
    AlertsManager* getAlertsManager() const { return alertsManager; }
    QString getProductVersion();

    /**
     * @brief Инициализация подключения к БД (вызывать после первоначальной настройки)
     */
    void ensureDbConnection();
    
    // Проверка подключения к БД
    bool isDatabaseConnected() const;

    // userid (users.id) -> employees.id для кассы
    qint64 getEmployeeIdByUserId(qint64 userId) const;
    
    // Управление сессией пользователя
    void setCurrentSession(const UserSession &session);
    UserSession getCurrentSession() const;
    bool hasActiveSession() const;

    /**
     * @brief Проверить токен сессии и обновить lastActivity при успехе
     * @return true если сессия валидна, false иначе (токен удалён из настроек, сессия очищена)
     */
    bool ensureSessionValid();

    /**
     * @brief Выход из учётной записи (удаляет токен, очищает сессию, испускает sessionInvalidated)
     */
    void logout();

signals:
    void sessionInvalidated();

private:
    DatabaseConnection* databaseConnection;
    StockManager* stockManager;
    ProductionManager* productionManager;
    ShiftManager* shiftManager;
    AlertsManager* alertsManager;
    SettingsManager* settingsManager;
    AuthManager* authManager;
    UserSession m_currentSession;
    QString m_productVersion;
    
    void CreateDbConnection(PostgreSQLAuth authConfig);
    void CloseDbConnection();
};

#endif // EASYPOSCORE_H
