#include "reportmanager.h"
#include "../db/databaseconnection.h"
#include "../sales/salesmanager.h"
#include "../sales/stockmanager.h"
#include "../sales/structures.h"
#include "../production/productionmanager.h"
#include "../production/structures.h"
#include "../shifts/shiftmanager.h"
#include "../shifts/structures.h"
#include <QSqlQuery>
#include <QLocale>
#include <QDateTime>

ReportManager::ReportManager(QObject *parent)
    : QObject(parent)
    , m_db(nullptr)
    , m_salesManager(nullptr)
    , m_stockManager(nullptr)
    , m_shiftManager(nullptr)
{
}

ReportManager::~ReportManager() = default;

void ReportManager::setDatabaseConnection(DatabaseConnection *conn)
{
    m_db = conn;
}

void ReportManager::setSalesManager(SalesManager *salesManager)
{
    m_salesManager = salesManager;
}

void ReportManager::setStockManager(StockManager *stockManager)
{
    m_stockManager = stockManager;
}

void ReportManager::setShiftManager(ShiftManager *shiftManager)
{
    m_shiftManager = shiftManager;
}

void ReportManager::setProductionManager(ProductionManager *productionManager)
{
    m_productionManager = productionManager;
}

ReportData ReportManager::generateSalesByPeriodReport(const QDate &dateFrom, const QDate &dateTo)
{
    ReportData data;
    data.title = QStringLiteral("Отчёт о продажах за период");
    data.suggestedFilename = QStringLiteral("Prodazhi_%1_%2").arg(dateFrom.toString(Qt::ISODate)).arg(dateTo.toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("№ чека")
        << QStringLiteral("Дата")
        << QStringLiteral("Время")
        << QStringLiteral("Сотрудник")
        << QStringLiteral("Сумма")
        << QStringLiteral("Скидка")
        << QStringLiteral("К оплате");

    if (!m_salesManager) return data;

    QList<Check> checks = m_salesManager->getChecks(dateFrom, dateTo, -1, false);
    double totalAmount = 0, totalDiscount = 0, totalToPay = 0;
    for (const Check &ch : checks) {
        double toPay = qMax(0.0, ch.totalAmount - ch.discountAmount);
        totalAmount += ch.totalAmount;
        totalDiscount += ch.discountAmount;
        totalToPay += toPay;
        data.rows.append(QStringList()
            << QString::number(ch.id)
            << ch.date.toString(Qt::ISODate)
            << ch.time.toString(QStringLiteral("hh:mm"))
            << ch.employeeName
            << QLocale().toString(ch.totalAmount, 'f', 2)
            << QLocale().toString(ch.discountAmount, 'f', 2)
            << QLocale().toString(toPay, 'f', 2));
    }
    data.summaryLines << QStringLiteral("Период: %1 — %2").arg(QLocale().toString(dateFrom, QLocale::ShortFormat)).arg(QLocale().toString(dateTo, QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество чеков: %1").arg(checks.size());
    data.summaryLines << QStringLiteral("Сумма продаж: %1 ₽").arg(QLocale().toString(totalAmount, 'f', 2));
    data.summaryLines << QStringLiteral("Сумма скидок: %1 ₽").arg(QLocale().toString(totalDiscount, 'f', 2));
    data.summaryLines << QStringLiteral("К оплате (итого): %1 ₽").arg(QLocale().toString(totalToPay, 'f', 2));
    return data;
}

ReportData ReportManager::generateStockReceiptReport(const QDate &dateFrom, const QDate &dateTo)
{
    ReportData data;
    data.title = QStringLiteral("Отчёт о принятии товаров и сырья на остатки");
    data.suggestedFilename = QStringLiteral("Prihod_%1_%2").arg(dateFrom.toString(Qt::ISODate)).arg(dateTo.toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("ID партии")
        << QStringLiteral("Товар")
        << QStringLiteral("Номер партии")
        << QStringLiteral("Количество")
        << QStringLiteral("Цена")
        << QStringLiteral("Дата изготовления")
        << QStringLiteral("Срок годности")
        << QStringLiteral("Дата обновления")
        << QStringLiteral("Сотрудник");

    if (!m_db || !m_db->isConnected()) return data;

    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT b.id, g.name, COALESCE(b.batchnumber,''), b.qnt, b.price, b.proddate, b.expdate, b.updatedate, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS empname "
        "FROM batches b "
        "JOIN goods g ON g.id = b.goodid "
        "LEFT JOIN employees e ON e.id = b.emoloyeeid "
        "WHERE (b.isdeleted IS NULL OR b.isdeleted = false) "
        "AND b.updatedate BETWEEN :dfrom AND :dto "
        "ORDER BY b.updatedate, b.id"));
    q.bindValue(QStringLiteral(":dfrom"), dateFrom);
    q.bindValue(QStringLiteral(":dto"), dateTo);
    if (!q.exec()) return data;

    qint64 totalQnt = 0;
    double totalValue = 0;
    while (q.next()) {
        qint64 qnt = q.value(3).toLongLong();
        double price = q.value(4).toDouble();
        totalQnt += qnt;
        totalValue += qnt * price;
        data.rows.append(QStringList()
            << q.value(0).toString()
            << q.value(1).toString()
            << q.value(2).toString()
            << q.value(3).toString()
            << QLocale().toString(price, 'f', 2)
            << q.value(5).toDate().toString(Qt::ISODate)
            << q.value(6).toDate().toString(Qt::ISODate)
            << q.value(7).toDate().toString(Qt::ISODate)
            << q.value(8).toString().trimmed());
    }
    data.summaryLines << QStringLiteral("Период: %1 — %2").arg(QLocale().toString(dateFrom, QLocale::ShortFormat)).arg(QLocale().toString(dateTo, QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество партий: %1").arg(data.rows.size());
    data.summaryLines << QStringLiteral("Всего единиц: %1").arg(totalQnt);
    data.summaryLines << QStringLiteral("Сумма по цене прихода: %1 ₽").arg(QLocale().toString(totalValue, 'f', 2));
    return data;
}

ReportData ReportManager::generateStockWriteOffReport(const QDate &dateFrom, const QDate &dateTo)
{
    ReportData data;
    data.title = QStringLiteral("Отчёт о списании с остатков");
    data.suggestedFilename = QStringLiteral("Spisanie_%1_%2").arg(dateFrom.toString(Qt::ISODate)).arg(dateTo.toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("Дата")
        << QStringLiteral("Время")
        << QStringLiteral("№ чека")
        << QStringLiteral("Товар")
        << QStringLiteral("Количество")
        << QStringLiteral("Сумма");

    if (!m_salesManager) return data;

    QList<Check> checks = m_salesManager->getChecks(dateFrom, dateTo, -1, false);
    double totalSum = 0;
    qint64 totalQnt = 0;
    for (const Check &ch : checks) {
        QList<SaleRow> rows = m_salesManager->getSalesByCheck(ch.id, false);
        for (const SaleRow &sale : rows) {
            if (sale.batchId == 0) continue;
            totalSum += sale.sum;
            totalQnt += sale.qnt;
            data.rows.append(QStringList()
                << ch.date.toString(Qt::ISODate)
                << ch.time.toString(QStringLiteral("hh:mm"))
                << QString::number(ch.id)
                << sale.itemName
                << QString::number(sale.qnt)
                << QLocale().toString(sale.sum, 'f', 2));
        }
    }
    data.summaryLines << QStringLiteral("Период: %1 — %2").arg(QLocale().toString(dateFrom, QLocale::ShortFormat)).arg(QLocale().toString(dateTo, QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество позиций списания: %1").arg(data.rows.size());
    data.summaryLines << QStringLiteral("Списано единиц: %1").arg(totalQnt);
    data.summaryLines << QStringLiteral("Сумма списания: %1 ₽").arg(QLocale().toString(totalSum, 'f', 2));
    return data;
}

ReportData ReportManager::generateSalesByShiftReport(qint64 shiftId)
{
    ReportData data;
    data.title = QStringLiteral("Отчёт о продажах за смену");
    data.suggestedFilename = QStringLiteral("Prodazhi_smena_%1").arg(shiftId);
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("№ чека")
        << QStringLiteral("Дата")
        << QStringLiteral("Время")
        << QStringLiteral("Сотрудник")
        << QStringLiteral("Сумма")
        << QStringLiteral("Скидка")
        << QStringLiteral("К оплате");

    if (!m_salesManager || shiftId <= 0) return data;

    QDate dfrom, dto;
    QString shiftInfo;
    if (m_shiftManager) {
        QList<WorkShift> shifts = m_shiftManager->getShifts(0, QDate(2000, 1, 1), QDate(2100, 12, 31));
        for (const WorkShift &ws : shifts) {
            if (ws.id == shiftId) {
                dfrom = ws.startedDate;
                dto = ws.endedDate.isValid() ? ws.endedDate : ws.startedDate;
                shiftInfo = tr("Смена №%1, %2 (%3 — %4)")
                    .arg(shiftId)
                    .arg(ws.employeeName.isEmpty() ? QString::number(ws.employeeId) : ws.employeeName)
                    .arg(ws.startedDate.toString(QLocale().dateFormat(QLocale::ShortFormat)))
                    .arg(ws.startedTime.toString(QStringLiteral("hh:mm")));
                break;
            }
        }
    }
    if (!dfrom.isValid()) dfrom = QDate::currentDate();
    if (!dto.isValid()) dto = dfrom;
    if (shiftInfo.isEmpty()) shiftInfo = tr("Смена №%1").arg(shiftId);

    QList<Check> checks = m_salesManager->getChecks(dfrom, dto, -1, false, shiftId);
    double totalAmount = 0, totalDiscount = 0, totalToPay = 0;
    for (const Check &ch : checks) {
        double toPay = qMax(0.0, ch.totalAmount - ch.discountAmount);
        totalAmount += ch.totalAmount;
        totalDiscount += ch.discountAmount;
        totalToPay += toPay;
        data.rows.append(QStringList()
            << QString::number(ch.id)
            << ch.date.toString(Qt::ISODate)
            << ch.time.toString(QStringLiteral("hh:mm"))
            << ch.employeeName
            << QLocale().toString(ch.totalAmount, 'f', 2)
            << QLocale().toString(ch.discountAmount, 'f', 2)
            << QLocale().toString(toPay, 'f', 2));
    }
    data.summaryLines << shiftInfo;
    data.summaryLines << QStringLiteral("Количество чеков: %1").arg(checks.size());
    data.summaryLines << QStringLiteral("Сумма продаж: %1 ₽").arg(QLocale().toString(totalAmount, 'f', 2));
    data.summaryLines << QStringLiteral("Сумма скидок: %1 ₽").arg(QLocale().toString(totalDiscount, 'f', 2));
    data.summaryLines << QStringLiteral("К оплате (итого): %1 ₽").arg(QLocale().toString(totalToPay, 'f', 2));
    return data;
}

ReportData ReportManager::generateReconciliationReport(qint64 shiftId)
{
    ReportData data;
    data.title = QStringLiteral("Отчёт о сверке итогов");
    data.suggestedFilename = QStringLiteral("Sverka_smena_%1").arg(shiftId);
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("№ чека")
        << QStringLiteral("Дата")
        << QStringLiteral("Время")
        << QStringLiteral("Сумма чека")
        << QStringLiteral("Скидка")
        << QStringLiteral("К оплате")
        << QStringLiteral("Способ оплаты")
        << QStringLiteral("Сумма оплаты");

    if (!m_salesManager || shiftId <= 0) return data;

    QDate dfrom, dto;
    QString shiftInfo;
    if (m_shiftManager) {
        QList<WorkShift> shifts = m_shiftManager->getShifts(0, QDate(2000, 1, 1), QDate(2100, 12, 31));
        for (const WorkShift &ws : shifts) {
            if (ws.id == shiftId) {
                dfrom = ws.startedDate;
                dto = ws.endedDate.isValid() ? ws.endedDate : ws.startedDate;
                shiftInfo = tr("Смена №%1, %2").arg(shiftId).arg(ws.employeeName.isEmpty() ? QString::number(ws.employeeId) : ws.employeeName);
                break;
            }
        }
    }
    if (!dfrom.isValid()) dfrom = QDate::currentDate();
    if (!dto.isValid()) dto = dfrom;
    if (shiftInfo.isEmpty()) shiftInfo = tr("Смена №%1").arg(shiftId);

    QList<Check> checks = m_salesManager->getChecks(dfrom, dto, -1, false, shiftId);
    double sumByChecks = 0, sumByPayments = 0;
    for (const Check &ch : checks) {
        double toPay = qMax(0.0, ch.totalAmount - ch.discountAmount);
        sumByChecks += toPay;
        QList<CheckPaymentRow> payments = m_salesManager->getCheckPayments(ch.id);
        if (payments.isEmpty()) {
            data.rows.append(QStringList()
                << QString::number(ch.id)
                << ch.date.toString(Qt::ISODate)
                << ch.time.toString(QStringLiteral("hh:mm"))
                << QLocale().toString(ch.totalAmount, 'f', 2)
                << QLocale().toString(ch.discountAmount, 'f', 2)
                << QLocale().toString(toPay, 'f', 2)
                << QString()
                << QString());
        } else {
            for (int i = 0; i < payments.size(); ++i) {
                const CheckPaymentRow &pay = payments.at(i);
                sumByPayments += pay.amount;
                data.rows.append(QStringList()
                    << QString::number(ch.id)
                    << ch.date.toString(Qt::ISODate)
                    << ch.time.toString(QStringLiteral("hh:mm"))
                    << QLocale().toString(ch.totalAmount, 'f', 2)
                    << QLocale().toString(ch.discountAmount, 'f', 2)
                    << QLocale().toString(toPay, 'f', 2)
                    << pay.paymentMethodName
                    << QLocale().toString(pay.amount, 'f', 2));
            }
        }
    }
    data.summaryLines << shiftInfo;
    data.summaryLines << QStringLiteral("Количество чеков: %1").arg(checks.size());
    data.summaryLines << QStringLiteral("Сумма к оплате по чекам: %1 ₽").arg(QLocale().toString(sumByChecks, 'f', 2));
    data.summaryLines << QStringLiteral("Сумма по оплатам: %1 ₽").arg(QLocale().toString(sumByPayments, 'f', 2));
    data.summaryLines << QStringLiteral("Расхождение: %1 ₽").arg(QLocale().toString(sumByPayments - sumByChecks, 'f', 2));
    return data;
}

ReportData ReportManager::generateStockBalanceReport()
{
    ReportData data;
    data.title = QStringLiteral("Выгрузка остатков");
    data.suggestedFilename = QStringLiteral("Ostatki_%1").arg(QDate::currentDate().toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("Товар")
        << QStringLiteral("Количество")
        << QStringLiteral("Зарезервировано")
        << QStringLiteral("Доступно")
        << QStringLiteral("Дата обновления");

    if (!m_stockManager) return data;

    QList<Stock> stocks = m_stockManager->getAllStocks(false);
    double totalQty = 0, totalReserved = 0, totalAvailable = 0;
    for (const Stock &s : stocks) {
        totalQty += s.quantity;
        totalReserved += s.reservedQuantity;
        totalAvailable += s.availableQuantity();
        data.rows.append(QStringList()
            << s.goodName
            << QLocale().toString(s.quantity, 'f', 2)
            << QLocale().toString(s.reservedQuantity, 'f', 2)
            << QLocale().toString(s.availableQuantity(), 'f', 2)
            << (s.updateDate.isValid() ? s.updateDate.toString(Qt::ISODate) : QString()));
    }
    data.summaryLines << QStringLiteral("На дату: %1").arg(QLocale().toString(QDate::currentDate(), QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество наименований: %1").arg(stocks.size());
    data.summaryLines << QStringLiteral("Всего на складе: %1").arg(QLocale().toString(totalQty, 'f', 2));
    data.summaryLines << QStringLiteral("Зарезервировано: %1").arg(QLocale().toString(totalReserved, 'f', 2));
    data.summaryLines << QStringLiteral("Доступно: %1").arg(QLocale().toString(totalAvailable, 'f', 2));
    return data;
}

ReportData ReportManager::generateStockBalanceWithBarcodesReport()
{
    ReportData data;
    data.title = QStringLiteral("Выгрузка остатков по партиям (со штрихкодами)");
    data.suggestedFilename = QStringLiteral("Ostatki_partii_%1").arg(QDate::currentDate().toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("Категория")
        << QStringLiteral("Товар")
        << QStringLiteral("Номер партии")
        << QStringLiteral("Количество")
        << QStringLiteral("Резерв")
        << QStringLiteral("Доступно")
        << QStringLiteral("Штрихкоды")
        << QStringLiteral("Дата обновления");

    if (!m_stockManager) return data;

    QList<BatchDetail> batches = m_stockManager->getBatches(false);
    qint64 totalQty = 0, totalReserved = 0;
    for (const BatchDetail &b : batches) {
        totalQty += b.qnt;
        totalReserved += b.reservedQuantity;
        QList<BarcodeEntry> barcodes = m_stockManager->getBarcodesForBatch(b.id, false);
        QString barcodeStr;
        for (const BarcodeEntry &e : barcodes) {
            if (!barcodeStr.isEmpty()) barcodeStr += QLatin1String(", ");
            barcodeStr += e.barcode;
        }
        data.rows.append(QStringList()
            << b.categoryName
            << b.goodName
            << b.batchNumber
            << QString::number(b.qnt)
            << QString::number(b.reservedQuantity)
            << QString::number(b.availableQuantity())
            << barcodeStr
            << (b.updateDate.isValid() ? QLocale().toString(b.updateDate, QLocale::ShortFormat) : QString()));
    }
    data.summaryLines << QStringLiteral("На дату: %1").arg(QLocale().toString(QDate::currentDate(), QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество партий: %1").arg(batches.size());
    data.summaryLines << QStringLiteral("Всего на складе: %1").arg(totalQty);
    data.summaryLines << QStringLiteral("Зарезервировано: %1").arg(totalReserved);
    return data;
}

ReportData ReportManager::generateChecksReport(const QDate &dateFrom, const QDate &dateTo, qint64 shiftId)
{
    ReportData data;
    if (shiftId > 0)
        data.title = QStringLiteral("Отчёт о чеках за смену");
    else
        data.title = QStringLiteral("Отчёт о чеках за период");
    data.suggestedFilename = (shiftId > 0)
        ? QStringLiteral("Cheki_smena_%1").arg(shiftId)
        : QStringLiteral("Cheki_%1_%2").arg(dateFrom.toString(Qt::ISODate)).arg(dateTo.toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("№ чека")
        << QStringLiteral("Дата")
        << QStringLiteral("Время")
        << QStringLiteral("Смена")
        << QStringLiteral("Сотрудник")
        << QStringLiteral("Сумма")
        << QStringLiteral("Скидка")
        << QStringLiteral("К оплате");

    if (!m_salesManager) return data;

    QList<Check> checks = m_salesManager->getChecks(dateFrom, dateTo, -1, false, shiftId > 0 ? shiftId : -1);
    double totalAmount = 0, totalDiscount = 0, totalToPay = 0;
    for (const Check &ch : checks) {
        double toPay = qMax(0.0, ch.totalAmount - ch.discountAmount);
        totalAmount += ch.totalAmount;
        totalDiscount += ch.discountAmount;
        totalToPay += toPay;
        data.rows.append(QStringList()
            << QString::number(ch.id)
            << ch.date.toString(Qt::ISODate)
            << ch.time.toString(QStringLiteral("hh:mm"))
            << (ch.shiftId > 0 ? QString::number(ch.shiftId) : QString())
            << ch.employeeName
            << QLocale().toString(ch.totalAmount, 'f', 2)
            << QLocale().toString(ch.discountAmount, 'f', 2)
            << QLocale().toString(toPay, 'f', 2));
    }
    if (shiftId > 0)
        data.summaryLines << QStringLiteral("Смена: %1").arg(shiftId);
    data.summaryLines << QStringLiteral("Период: %1 — %2").arg(QLocale().toString(dateFrom, QLocale::ShortFormat)).arg(QLocale().toString(dateTo, QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество чеков: %1").arg(checks.size());
    data.summaryLines << QStringLiteral("Сумма продаж: %1 ₽").arg(QLocale().toString(totalAmount, 'f', 2));
    data.summaryLines << QStringLiteral("Сумма скидок: %1 ₽").arg(QLocale().toString(totalDiscount, 'f', 2));
    data.summaryLines << QStringLiteral("К оплате (итого): %1 ₽").arg(QLocale().toString(totalToPay, 'f', 2));
    return data;
}

ReportData ReportManager::generateProductionReport(const QDate &dateFrom, const QDate &dateTo)
{
    ReportData data;
    data.title = QStringLiteral("Отчёт о произведённой продукции");
    data.suggestedFilename = QStringLiteral("Proizvodstvo_%1_%2").arg(dateFrom.toString(Qt::ISODate)).arg(dateTo.toString(Qt::ISODate));
    data.generatedAt = QDateTime::currentDateTime();
    data.header = QStringList()
        << QStringLiteral("Дата")
        << QStringLiteral("Рецепт / Продукция")
        << QStringLiteral("Кол-во")
        << QStringLiteral("Сотрудник")
        << QStringLiteral("Партия");

    if (!m_productionManager) return data;

    QList<ProductionRun> runs = m_productionManager->getProductionRuns(dateFrom, dateTo, -1, false);
    double totalQty = 0;
    for (const ProductionRun &r : runs) {
        totalQty += r.quantityProduced;
        data.rows.append(QStringList()
            << r.productionDate.toString(Qt::ISODate)
            << r.recipeName
            << QLocale().toString(r.quantityProduced, 'f', 2)
            << r.employeeName
            << r.outputBatchNumber);
    }
    data.summaryLines << QStringLiteral("Период: %1 — %2").arg(QLocale().toString(dateFrom, QLocale::ShortFormat)).arg(QLocale().toString(dateTo, QLocale::ShortFormat));
    data.summaryLines << QStringLiteral("Количество актов производства: %1").arg(runs.size());
    data.summaryLines << QStringLiteral("Всего произведено (ед.): %1").arg(QLocale().toString(totalQty, 'f', 2));
    return data;
}
