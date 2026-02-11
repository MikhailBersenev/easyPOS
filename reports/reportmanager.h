#ifndef REPORTMANAGER_H
#define REPORTMANAGER_H

#include "reportdata.h"
#include <QObject>
#include <QDate>

class DatabaseConnection;
class SalesManager;
class StockManager;
class ShiftManager;
class ProductionManager;

/**
 * Менеджер отчётов. Формирует данные отчётов (ReportData),
 * экспорт в нужный формат выполняется через AbstractReportExporter (например, CsvReportExporter).
 */
class ReportManager : public QObject
{
    Q_OBJECT
public:
    explicit ReportManager(QObject *parent = nullptr);
    ~ReportManager();

    void setDatabaseConnection(DatabaseConnection *conn);
    void setSalesManager(SalesManager *salesManager);
    void setStockManager(StockManager *stockManager);
    void setShiftManager(ShiftManager *shiftManager);
    void setProductionManager(ProductionManager *productionManager);

    /** Отчёт о продажах за указанный период */
    ReportData generateSalesByPeriodReport(const QDate &dateFrom, const QDate &dateTo);

    /** Отчёт о принятии товаров и сырья на остатки (партии за период по дате обновления) */
    ReportData generateStockReceiptReport(const QDate &dateFrom, const QDate &dateTo);

    /** Отчёт о списании с остатков (продажи товаров за период) */
    ReportData generateStockWriteOffReport(const QDate &dateFrom, const QDate &dateTo);

    /** Отчёт о продажах за указанную смену */
    ReportData generateSalesByShiftReport(qint64 shiftId);

    /** Отчёт о сверке итогов (по чекам смены: сумма чека и оплаты по способам) */
    ReportData generateReconciliationReport(qint64 shiftId);

    /** Выгрузка остатков (по товарам, сводно) */
    ReportData generateStockBalanceReport();

    /** Выгрузка остатков по партиям, включая штрихкоды */
    ReportData generateStockBalanceWithBarcodesReport();

    /** Отчёт о чеках за смену или период (shiftId <= 0 — за период) */
    ReportData generateChecksReport(const QDate &dateFrom, const QDate &dateTo, qint64 shiftId = -1);

    /** Отчёт о произведённой продукции за период */
    ReportData generateProductionReport(const QDate &dateFrom, const QDate &dateTo);

private:
    DatabaseConnection *m_db = nullptr;
    SalesManager *m_salesManager = nullptr;
    StockManager *m_stockManager = nullptr;
    ShiftManager *m_shiftManager = nullptr;
    ProductionManager *m_productionManager = nullptr;
};

#endif // REPORTMANAGER_H
