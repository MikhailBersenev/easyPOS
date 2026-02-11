#include "stockbalancedialog.h"
#include "batcheditdialog.h"
#include "reportwizarddialog.h"
#include "../easyposcore.h"
#include "../sales/stockmanager.h"
#include "../sales/structures.h"
#include "../RBAC/structures.h"
#include <QDate>
#include <QInputDialog>
#include <QLocale>
#include <QMessageBox>
#include <QSqlQuery>

StockBalanceDialog::StockBalanceDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Остатки на складе"))
{
    setupColumns({ tr("ID"), tr("Товар"), tr("Номер партии"), tr("Кол-во"), tr("Резерв"), tr("Цена"),
                  tr("Дата пр-ва"), tr("Срок годности"), tr("Списана") },
                 { 50, 180, 120, 70, 70, 80, 90, 90, 60 });
    setReportButtonVisible(true);
    setReportButtonText(tr("Мастер отчётов"));
    connect(this, &ReferenceDialog::reportButtonClicked, this, &StockBalanceDialog::onExportReport);
    loadTable();
}

StockManager *StockBalanceDialog::stockManager() const
{
    return m_core ? m_core->getStockManager() : nullptr;
}

QList<QPair<qint64, QString>> StockBalanceDialog::loadGoodsList() const
{
    QList<QPair<qint64, QString>> list;
    if (!m_core || !m_core->getDatabaseConnection())
        return list;
    QSqlQuery q(m_core->getDatabaseConnection()->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT id, name FROM goods WHERE (isdeleted IS NULL OR isdeleted = false) AND isactive = true ORDER BY name"));
    if (!q.exec())
        return list;
    while (q.next())
        list.append({ q.value(0).toLongLong(), q.value(1).toString() });
    return list;
}

void StockBalanceDialog::loadTable(bool includeDeleted)
{
    clearTable();
    StockManager *sm = stockManager();
    if (!sm)
        return;

    const QList<BatchDetail> batches = sm->getBatches(includeDeleted);
    for (const BatchDetail &b : batches) {
        const int row = insertRow();
        setCellText(row, 0, QString::number(b.id));
        setCellText(row, 1, b.goodName);
        setCellText(row, 2, b.batchNumber);
        setCellText(row, 3, QString::number(b.qnt));
        setCellText(row, 4, QString::number(b.reservedQuantity));
        setCellText(row, 5, QString::number(b.price));
        setCellText(row, 6, b.prodDate.isValid() ? QLocale().toString(b.prodDate, QLocale::ShortFormat) : QString());
        setCellText(row, 7, b.expDate.isValid() ? QLocale().toString(b.expDate, QLocale::ShortFormat) : QString());
        setCellText(row, 8, b.writtenOff ? tr("Да") : tr("Нет"));
    }
    updateRecordCount();
}

void StockBalanceDialog::onExportReport()
{
    if (!m_core) return;
    QDate today = QDate::currentDate();
    ReportWizardDialog dlg(this, m_core);
    dlg.setPreset(ReportStockBalanceWithBarcodes, today, today, 0, 2);
    dlg.exec();
}

void StockBalanceDialog::addRecord()
{
    StockManager *sm = stockManager();
    if (!sm) {
        showError(tr("Менеджер склада не доступен."));
        return;
    }

    const QList<QPair<qint64, QString>> goods = loadGoodsList();
    if (goods.isEmpty()) {
        showError(tr("Нет товаров в справочнике."));
        return;
    }

    UserSession session = m_core->getCurrentSession();
    qint64 employeeId = m_core->getEmployeeIdByUserId(session.userId);
    if (employeeId <= 0) {
        showError(tr("Нет привязки пользователя к сотруднику."));
        return;
    }

    BatchEditDialog dlg(this);
    dlg.setGoodsList(goods);
    dlg.setWindowTitle(tr("Новая партия"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    StockOperationResult result = sm->createBatch(dlg.goodId(), dlg.batchNumber(), dlg.quantity(),
                                                  dlg.price(), dlg.prodDate(), dlg.expDate(), employeeId);
    if (!result.success) {
        showError(result.message);
        return;
    }
    const qint64 newBatchId = result.stockId;
    for (const QString &bc : dlg.barcodesToAdd()) {
        sm->addBarcodeToBatch(newBatchId, bc);
    }
    showInfo(tr("Партия создана."));
    loadTable();
}

void StockBalanceDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0) {
        showWarning(tr("Выберите партию для редактирования."));
        return;
    }

    StockManager *sm = stockManager();
    if (!sm) {
        showError(tr("Менеджер склада не доступен."));
        return;
    }

    const qint64 batchId = cellText(row, 0).toLongLong();
    BatchDetail batch = sm->getBatch(batchId);
    if (batch.id == 0) {
        showError(tr("Партия не найдена."));
        return;
    }

    const QList<QPair<qint64, QString>> goods = loadGoodsList();
    BatchEditDialog dlg(this);
    dlg.setGoodsList(goods);
    dlg.setBatch(batch);
    dlg.setBarcodes(sm->getBarcodesForBatch(batchId));
    if (dlg.exec() != QDialog::Accepted)
        return;

    StockOperationResult result = sm->updateBatch(batchId, dlg.batchNumber(), dlg.quantity(),
                                                   dlg.reservedQuantity(), dlg.price(),
                                                   dlg.prodDate(), dlg.expDate(), dlg.writtenOff());
    if (!result.success) {
        showError(result.message);
        return;
    }
    for (qint64 barcodeId : dlg.barcodeIdsToRemove()) {
        sm->removeBarcode(barcodeId);
    }
    for (const QString &bc : dlg.barcodesToAdd()) {
        sm->addBarcodeToBatch(batchId, bc);
    }
    showInfo(tr("Партия обновлена."));
    loadTable();
}

void StockBalanceDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;

    const qint64 batchId = cellText(row, 0).toLongLong();
    const QString goodName = cellText(row, 1);
    const QString batchNumber = cellText(row, 2);

    if (!askConfirmation(tr("Пометить партию «%1» (товар: %2) как удалённую?").arg(batchNumber, goodName)))
        return;

    StockManager *sm = stockManager();
    if (!sm)
        return;

    StockOperationResult result = sm->setBatchDeleted(batchId, true);
    if (result.success) {
        showInfo(tr("Партия помечена как удалённая."));
        loadTable();
    } else {
        showError(result.message);
    }
}
