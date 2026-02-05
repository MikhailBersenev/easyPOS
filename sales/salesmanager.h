#ifndef SALESMANAGER_H
#define SALESMANAGER_H

#include <QObject>
#include <QList>
#include <QSqlError>
#include "../db/databaseconnection.h"
#include "structures.h"

class SalesManager : public QObject
{
    Q_OBJECT
public:
    explicit SalesManager(QObject *parent = nullptr);
    ~SalesManager();

    void setDatabaseConnection(DatabaseConnection *dbConnection);
    void setStockManager(class StockManager *stockManager);

    // Чек
    /** Создать чек. shiftId — ID активной смены сотрудника (обязательно для привязки чека к смене). */
    SaleOperationResult createCheck(qint64 employeeId, qint64 shiftId = 0);
    Check getCheck(qint64 checkId, bool includeDeleted = false);
    /** shiftId &lt;= 0 — все смены, иначе только чеки указанной смены */
    QList<Check> getChecks(const QDate &dateFrom, const QDate &dateTo,
                           qint64 employeeId = -1, bool includeDeleted = false, qint64 shiftId = -1);
    SaleOperationResult setCheckDiscount(qint64 checkId, double discountAmount);
    SaleOperationResult cancelCheck(qint64 checkId);
    SaleOperationResult finalizeCheck(qint64 checkId);

    // Позиции продаж
    SaleOperationResult addSaleByBatchId(qint64 checkId, qint64 batchId, qint64 qnt,
                                        qint64 vatRateId = 0);
    SaleOperationResult addSaleByServiceId(qint64 checkId, qint64 serviceId, qint64 qnt,
                                          qint64 vatRateId = 0);
    SaleOperationResult addSale(qint64 checkId, qint64 itemId, qint64 qnt,
                               qint64 vatRateId = 0);
    SaleOperationResult removeSale(qint64 saleId);
    SaleOperationResult updateSaleQuantity(qint64 saleId, qint64 newQnt);

    QList<SaleRow> getSalesByCheck(qint64 checkId, bool includeDeleted = false);
    SaleRow getSale(qint64 saleId, bool includeDeleted = false);

    // Справочники
    qint64 ensureDefaultVatRate();
    QList<BatchInfo> getAvailableBatches();
    QList<ServiceInfo> getAvailableServices();

    // Способы оплаты и оплаты по чеку
    QList<PaymentMethodInfo> getPaymentMethods();
    SaleOperationResult recordCheckPayments(qint64 checkId, const QList<CheckPaymentRow> &payments);

    QSqlError lastError() const;

signals:
    void checkCreated(qint64 checkId, qint64 employeeId);
    void checkCancelled(qint64 checkId);
    void checkFinalized(qint64 checkId, double totalAmount);
    void saleAdded(qint64 checkId, qint64 saleId, qint64 itemId, qint64 qnt, double sum);
    void saleRemoved(qint64 checkId, qint64 saleId);
    void operationFailed(const QString &message);

private:
    DatabaseConnection *m_dbConnection;
    class StockManager *m_stockManager;
    QSqlError m_lastError;

    void recalcCheckTotal(qint64 checkId);
    qint64 getOrCreateItemForBatch(qint64 batchId);
    qint64 getOrCreateItemForService(qint64 serviceId);
    double getBatchPrice(qint64 batchId);
    double getServicePrice(qint64 serviceId);
    qint64 resolveVatRateId(qint64 vatRateId);
};

#endif // SALESMANAGER_H
