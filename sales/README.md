# StockManager - Управление остатками товаров

## Описание

`StockManager` - это класс для управления остатками товаров на складе. Он предоставляет методы для создания, обновления, списания и резервирования товаров.

## Основные возможности

- ✅ Установка остатка товара
- ✅ Добавление товара на склад
- ✅ Списание товара со склада
- ✅ Резервирование товара
- ✅ Снятие резерва
- ✅ Получение информации об остатках
- ✅ Проверка достаточности товара
- ✅ Мягкое удаление остатков

## Быстрый старт

### 1. Создание StockManager

```cpp
#include "sales/stockmanager.h"
#include "easyposcore.h"

// Создание через EasyPOSCore
std::shared_ptr<EasyPOSCore> core = std::make_shared<EasyPOSCore>();
StockManager* stockManager = core->createStockManager();
```

### 2. Установка остатка

```cpp
// По ID товара
StockOperationResult result = stockManager->setStock(1, 100.0);
if (result.success) {
    qDebug() << result.message; // "Остаток успешно создан" или "Остаток успешно обновлен"
}

// По названию товара
result = stockManager->setStock("Хлеб белый", 50.0);
```

### 3. Добавление товара на склад

```cpp
// Увеличивает существующее количество
result = stockManager->addStock(1, 25.0); // Добавить 25 единиц
```

### 4. Списание товара

```cpp
// Уменьшает количество, проверяет достаточность
result = stockManager->removeStock(1, 10.0); // Списать 10 единиц
if (!result.success) {
    qDebug() << "Ошибка:" << result.message; // Может быть "Недостаточно товара"
}
```

### 5. Резервирование товара

```cpp
// Резервирует количество, но не списывает
result = stockManager->reserveStock(1, 5.0); // Зарезервировать 5 единиц
```

### 6. Снятие резерва

```cpp
result = stockManager->releaseReserve(1, 3.0); // Снять резерв с 3 единиц
```

### 7. Получение информации об остатке

```cpp
// По ID остатка
Stock stock = stockManager->getStock(1);

// По ID товара
stock = stockManager->getStockByGoodId(1);

// По названию товара
stock = stockManager->getStockByGoodName("Хлеб белый");

if (stock.id > 0) {
    qDebug() << "Товар:" << stock.goodName;
    qDebug() << "Количество:" << stock.quantity;
    qDebug() << "Зарезервировано:" << stock.reservedQuantity;
    qDebug() << "Доступно:" << stock.availableQuantity(); // quantity - reservedQuantity
}
```

### 8. Проверка достаточности

```cpp
bool hasEnough = stockManager->hasEnoughStock(1, 50.0);
if (hasEnough) {
    // Можно списать товар
} else {
    // Товара недостаточно
}
```

### 9. Получение всех остатков

```cpp
QList<Stock> allStocks = stockManager->getAllStocks();
for (const Stock& s : allStocks) {
    qDebug() << s.goodName << ":" << s.quantity 
             << "(доступно:" << s.availableQuantity() << ")";
}
```

### 10. Удаление и восстановление

```cpp
// Мягкое удаление (устанавливает isdeleted = true)
result = stockManager->deleteStock(1);

// Восстановление
result = stockManager->restoreStock(1);
```

## Использование сигналов

```cpp
// Подключение к сигналам для отслеживания событий
connect(stockManager, &StockManager::stockUpdated,
        [](int stockId, int goodId, double quantity) {
    qDebug() << "Остаток обновлен: stockId=" << stockId;
});

connect(stockManager, &StockManager::stockAdded,
        [](int stockId, int goodId, double quantity) {
    qDebug() << "Товар добавлен на склад";
});

connect(stockManager, &StockManager::stockRemoved,
        [](int stockId, int goodId, double quantity) {
    qDebug() << "Товар списан со склада";
});

connect(stockManager, &StockManager::lowStockWarning,
        [](int goodId, double currentQuantity, double minThreshold) {
    qDebug() << "ВНИМАНИЕ: Низкий остаток!";
});

connect(stockManager, &StockManager::operationFailed,
        [](const QString &message) {
    qDebug() << "Ошибка операции:" << message;
});
```

## Структура Stock

```cpp
struct Stock {
    int id;                 // ID записи об остатке
    int goodId;             // ID товара
    QString goodName;       // Название товара
    double quantity;        // Общее количество
    double reservedQuantity; // Зарезервированное количество
    QDateTime updateDate;   // Дата обновления
    bool isDeleted;         // Флаг удаления
    
    // Методы
    double availableQuantity() const; // Доступное количество (quantity - reservedQuantity)
    bool hasEnough(double required) const; // Проверка достаточности
};
```

## Структура StockOperationResult

```cpp
struct StockOperationResult {
    bool success;           // Успешность операции
    QString message;        // Сообщение о результате
    int stockId;            // ID созданной/обновленной записи
    QSqlError error;        // Ошибка SQL (если есть)
};
```

## Обработка ошибок

Все методы возвращают `StockOperationResult`. Всегда проверяйте `success`:

```cpp
StockOperationResult result = stockManager->setStock(1, 100.0);
if (result.success) {
    // Операция успешна
    qDebug() << result.message;
} else {
    // Ошибка
    qDebug() << "Ошибка:" << result.message;
    if (result.error.isValid()) {
        qDebug() << "SQL ошибка:" << result.error.text();
    }
}
```

## Пример использования в классе окна

```cpp
class MyWindow : public QWidget
{
    Q_OBJECT
    
public:
    MyWindow(std::shared_ptr<EasyPOSCore> core, QWidget *parent = nullptr)
        : QWidget(parent), m_core(core)
    {
        m_stockManager = m_core->createStockManager(this);
        
        connect(m_stockManager, &StockManager::stockUpdated,
                this, &MyWindow::onStockUpdated);
    }
    
private slots:
    void onStockUpdated(int stockId, int goodId, double quantity)
    {
        // Обновить UI
    }
    
private:
    std::shared_ptr<EasyPOSCore> m_core;
    StockManager* m_stockManager;
};
```

## Важные замечания

1. **Подключение к БД**: `StockManager` требует активного подключения к базе данных через `EasyPOSCore`.

2. **Таблица batches**: `StockManager` работает с таблицей `batches` (партии товаров). Остатки агрегируются по `goodid`:
   - `goodid` (bigint, foreign key к goods)
   - `qnt` (bigint) — количество в партии
   - `reservedquantity` (bigint) — зарезервированное количество (добавлено миграцией)
   - `isdeleted` (bool) — мягкое удаление
   - `updatedate` (date) — дата обновления
   - `proddate`, `expdate` — даты производства и срока годности
   - `price` — цена партии
   - `emoloyeeid` — сотрудник, принявший партию

3. **Миграция**: Перед использованием выполни `db/migrate_add_reservedquantity.sql` для добавления поля `reservedquantity` в `batches`.

4. **Агрегация остатков**: `getStockByGoodId` возвращает агрегированные данные: `SUM(qnt)` и `SUM(reservedquantity)` по всем партиям товара. `Stock.id` = `goodId` (виртуальный ID).

5. **Списание (FIFO)**: `removeStock` списывает из партий по принципу FIFO (сначала старые по `proddate`).

6. **Резервирование**: Распределяется по партиям (FIFO), уменьшает доступное количество без физического списания.

7. **Доступное количество**: Используйте `stock.availableQuantity()` для получения количества, доступного для списания (общее минус зарезервированное).

---

# SalesManager — продажи (чеки и позиции)

## Описание

`SalesManager` работает с таблицами `checks`, `sales`, `items`, `batches`, `services`, `vatrates`. Создаёт чеки, добавляет позиции по партиям товаров или услугам, пересчитывает итоги, отменяет чеки с возвратом в партии.

## Основные возможности

- Создание чека (`createCheck`), получение чека и списка чеков за период
- Добавление позиций: по `batchId` (`addSaleByBatchId`), по `serviceId` (`addSaleByServiceId`), по `itemId` (`addSale`)
- Удаление позиции (`removeSale`), отмена чека (`cancelCheck`)
- Скидка на чек (`setCheckDiscount`)
- Справочник НДС: при отсутствии ставок создаётся «Без НДС» 0%

## Быстрый старт

```cpp
#include "sales/salesmanager.h"
#include "easyposcore.h"

auto core = std::make_shared<EasyPOSCore>();
SalesManager* sm = core->createSalesManager(this);
// Подключение к БД уже задаётся в createSalesManager.

// Создать чек от лица сотрудника
SaleOperationResult r = sm->createCheck(employeeId);
if (!r.success) { /* r.message, r.error */ return; }
qint64 checkId = r.checkId;

// Добавить позицию по партии (товар)
r = sm->addSaleByBatchId(checkId, batchId, qnt);
// или по услуге
r = sm->addSaleByServiceId(checkId, serviceId, qnt);
// или по itemId (номенклатура уже есть)
r = sm->addSale(checkId, itemId, qnt);

// Скидка на чек
sm->setCheckDiscount(checkId, 50.0);

// Список позиций чека
QList<SaleRow> rows = sm->getSalesByCheck(checkId);

// Удалить позицию, отменить чек
sm->removeSale(saleId);
sm->cancelCheck(checkId);
```

## Структуры

- **Check**: id, date, time, totalAmount, discountAmount, employeeId, employeeName, isDeleted
- **SaleRow**: id, itemId, qnt, vatRateId, sum, checkId, batchId, serviceId, itemName, unitPrice, ...
- **SaleOperationResult**: success, message, checkId, saleId, error
