#ifndef SALES_STRUCTURES_H
#define SALES_STRUCTURES_H

#include <QString>
#include <QDate>
#include <QDate>
#include <QTime>
#include <QSqlError>

// Остаток товара на складе
struct Stock {
    int id;                 // ID записи об остатке
    int goodId;             // ID товара
    QString goodName;       // Название товара (для удобства)
    double quantity;        // Количество товара
    double reservedQuantity; // Зарезервированное количество
    QDate updateDate;   // Дата последнего обновления
    QTime updateTime;  // Время последнего обновления
    bool isDeleted;         // Флаг удаления
    
    Stock() : id(0), goodId(0), quantity(0.0), reservedQuantity(0.0), isDeleted(false) {}
    
    // Доступное количество (общее минус зарезервированное)
    double availableQuantity() const {
        return quantity - reservedQuantity;
    }
    
    // Проверка наличия достаточного количества
    bool hasEnough(double required) const {
        return availableQuantity() >= required;
    }
};

// Чек (продажа) — таблица checks
struct Check {
    qint64 id = 0;
    QDate date;
    QTime time;
    double totalAmount = 0.0;
    double discountAmount = 0.0;
    qint64 employeeId = 0;
    qint64 shiftId = 0;  // смена, в которой создан чек
    QString employeeName;
    bool isDeleted = false;
};

// Позиция продажи — таблица sales
struct SaleRow {
    qint64 id = 0;
    qint64 itemId = 0;
    qint64 qnt = 0;
    qint64 vatRateId = 0;
    double sum = 0.0;
    QString comment;
    qint64 checkId = 0;
    bool isDeleted = false;
    // Для отображения (из item -> batch/service + good)
    QString itemName;   // название товара или услуги
    qint64 batchId = 0;
    qint64 serviceId = 0;
    double unitPrice = 0.0;
};

// Результат операции продаж
struct SaleOperationResult {
    bool success = false;
    QString message;
    qint64 checkId = 0;
    qint64 saleId = 0;
    QSqlError error;
};

// Партия для кассы (товар в наличии)
struct BatchInfo {
    qint64 batchId = 0;
    qint64 goodId = 0;
    QString goodName;
    QString categoryName;
    double price = 0.0;
    qint64 qnt = 0;
    QString batchNumber;
    QString imageUrl;  // goods.imageurl — путь/URL к пиктограмме
};

// Услуга для кассы
struct ServiceInfo {
    qint64 serviceId = 0;
    QString serviceName;
    QString categoryName;
    double price = 0.0;
};

// Способ оплаты
struct PaymentMethodInfo {
    qint64 id = 0;
    QString name;
    int sortOrder = 0;
    bool isActive = true;
};

// Оплата по чеку (одна строка)
struct CheckPaymentRow {
    qint64 paymentMethodId = 0;
    QString paymentMethodName;
    double amount = 0.0;
};

// Партия товара (для справочника остатков / редактирования)
struct BatchDetail {
    qint64 id = 0;
    qint64 goodId = 0;
    QString goodName;
    QString batchNumber;
    qint64 qnt = 0;
    qint64 reservedQuantity = 0;
    double price = 0.0;
    QDate prodDate;
    QDate expDate;
    bool writtenOff = false;
    QDate updateDate;
    qint64 employeeId = 0;
    bool isDeleted = false;

    qint64 availableQuantity() const { return qnt - reservedQuantity; }
};

// Штрихкод партии
struct BarcodeEntry {
    qint64 id = 0;
    QString barcode;
    qint64 batchId = 0;
    QDate updateDate;
    bool isDeleted = false;
};

#endif // SALES_STRUCTURES_H

