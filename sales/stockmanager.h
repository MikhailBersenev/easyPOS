#ifndef STOCKMANAGER_H
#define STOCKMANAGER_H

#include <QObject>
#include <QString>
#include <QSqlError>
#include <QList>
#include "../db/databaseconnection.h"
#include "structures.h"

// Результат операции с остатками
struct StockOperationResult {
    bool success;
    QString message;
    int stockId;  // ID созданной/обновленной записи об остатке
    QSqlError error;
    
    StockOperationResult() : success(false), stockId(0) {}
};

class StockManager : public QObject
{
    Q_OBJECT
public:
    explicit StockManager(QObject *parent = nullptr);
    ~StockManager();

    // Установка подключения к базе данных
    void setDatabaseConnection(DatabaseConnection *dbConnection);
    
    // Создание/обновление остатка товара
    StockOperationResult setStock(int goodId, double quantity);
    StockOperationResult setStock(const QString &goodName, double quantity);
    
    // Добавление товара на склад (увеличение количества)
    StockOperationResult addStock(int goodId, double quantity);
    StockOperationResult addStock(const QString &goodName, double quantity);
    
    // Списание товара со склада (уменьшение количества)
    StockOperationResult removeStock(int goodId, double quantity);
    StockOperationResult removeStock(const QString &goodName, double quantity);
    
    // Резервирование товара
    StockOperationResult reserveStock(int goodId, double quantity);
    StockOperationResult reserveStock(const QString &goodName, double quantity);
    
    // Снятие резерва товара
    StockOperationResult releaseReserve(int goodId, double quantity);
    StockOperationResult releaseReserve(const QString &goodName, double quantity);
    
    // Удаление остатка (мягкое удаление - установка isdeleted = true)
    // stockId здесь = goodId (остаток агрегируется по товару)
    StockOperationResult deleteStock(int goodId);
    
    // Восстановление удаленного остатка
    StockOperationResult restoreStock(int stockId);
    
    // Получение остатка по ID
    Stock getStock(int stockId, bool includeDeleted = false);
    
    // Получение остатка по ID товара
    Stock getStockByGoodId(int goodId, bool includeDeleted = false);
    
    // Получение остатка по названию товара
    Stock getStockByGoodName(const QString &goodName, bool includeDeleted = false);
    
    // Получение всех остатков
    QList<Stock> getAllStocks(bool includeDeleted = false);
    
    // Получение остатков по ID товара (может быть несколько записей)
    QList<Stock> getStocksByGoodId(int goodId, bool includeDeleted = false);
    
    // Проверка наличия достаточного количества товара
    bool hasEnoughStock(int goodId, double requiredQuantity);
    bool hasEnoughStock(const QString &goodName, double requiredQuantity);
    
    // Проверка существования остатка
    bool stockExists(int stockId, bool includeDeleted = false);
    bool stockExistsByGoodId(int goodId, bool includeDeleted = false);

    // Партии (для справочника остатков)
    QList<BatchDetail> getBatches(bool includeDeleted = false);
    BatchDetail getBatch(qint64 batchId);
    StockOperationResult updateBatch(qint64 batchId, const QString &batchNumber, qint64 qnt,
                                    qint64 reservedQuantity, double price,
                                    const QDate &prodDate, const QDate &expDate, bool writtenOff);
    StockOperationResult createBatch(qint64 goodId, const QString &batchNumber, qint64 qnt,
                                     double price, const QDate &prodDate, const QDate &expDate,
                                     qint64 employeeId);
    StockOperationResult setBatchDeleted(qint64 batchId, bool deleted);

    // Штрихкоды партии
    QList<BarcodeEntry> getBarcodesForBatch(qint64 batchId, bool includeDeleted = false);
    StockOperationResult addBarcodeToBatch(qint64 batchId, const QString &barcode);
    StockOperationResult removeBarcode(qint64 barcodeId);
    
    // Получение последней ошибки
    QSqlError lastError() const;

signals:
    // Сигналы для уведомления о событиях
    void stockUpdated(int stockId, int goodId, double quantity);
    void stockAdded(int stockId, int goodId, double quantity);
    void stockRemoved(int stockId, int goodId, double quantity);
    void stockReserved(int stockId, int goodId, double quantity);
    void stockReleased(int stockId, int goodId, double quantity);
    void stockDeleted(int stockId, int goodId);
    void stockRestored(int stockId, int goodId);
    void operationFailed(const QString &message);
    void lowStockWarning(int goodId, double currentQuantity, double minThreshold);

private:
    DatabaseConnection *m_dbConnection;
    QSqlError m_lastError;
    
    // Вспомогательные методы
    int getGoodIdByName(const QString &goodName);
    bool goodExists(int goodId);
    
    // Внутренний метод для установки isdeleted
    StockOperationResult setStockDeletedStatus(int stockId, bool isDeleted);
    
    // Внутренний метод для создания/обновления остатка
    StockOperationResult upsertStock(int goodId, double quantity, bool addToExisting = false);
};

#endif // STOCKMANAGER_H

