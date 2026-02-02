#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "easyposcore.h"
#include "RBAC/structures.h"
#include "sales/salesmanager.h"
#include "sales/structures.h"
#include "ui/sales/goodfind.h"
#include "ui/goodcatsdialog.h"
#include "ui/goodsdialog.h"
#include "ui/windowcreator.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> core, const UserSession &session)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_core(core)
    , m_session(session)
{
    ui->setupUi(this);

    m_employeeId = m_core ? m_core->getEmployeeIdByUserId(m_session.userId) : 0;
    m_salesManager = m_core ? m_core->createSalesManager(this) : nullptr;

    if (!m_salesManager || m_employeeId <= 0) {
        setPosEnabled(false);
        if (m_employeeId <= 0)
            ui->statusbar->showMessage(tr("Нет привязки пользователя к сотруднику. Касса недоступна."));
        else
            ui->statusbar->showMessage(tr("Нет подключения к БД или SalesManager."));
    } else {
        setPosEnabled(true);
        ui->statusbar->showMessage(tr("Касса готова. Нажмите «Новый чек» для начала продажи."));
    }

    ui->checkWidget->insertColumn(4);
    ui->checkWidget->setColumnHidden(4, true);
    // Слоты подключены автоматически через соглашение об именованию Qt (on_<objectName>_<signal>)
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setPosEnabled(bool enabled)
{
    ui->newCheckButton->setEnabled(enabled);
    ui->findGoodButton->setEnabled(enabled);
    ui->removeFromCartButton->setEnabled(enabled);
    ui->discountButton->setEnabled(enabled);
    ui->payButton->setEnabled(enabled);
    ui->checkWidget->setEnabled(enabled);
    ui->qtySpinBox->setEnabled(enabled);
}

void MainWindow::refreshCart()
{
    ui->checkWidget->setRowCount(0);
    if (!m_salesManager || m_currentCheckId <= 0) {
        updateTotals();
        return;
    }
    const QList<SaleRow> rows = m_salesManager->getSalesByCheck(m_currentCheckId);
    for (const SaleRow &r : rows) {
        const int row = ui->checkWidget->rowCount();
        ui->checkWidget->insertRow(row);
        ui->checkWidget->setItem(row, 0, new QTableWidgetItem(r.itemName));
        ui->checkWidget->setItem(row, 1, new QTableWidgetItem(QString::number(r.qnt)));
        ui->checkWidget->setItem(row, 2, new QTableWidgetItem(QString::number(r.unitPrice, 'f', 2)));
        ui->checkWidget->setItem(row, 3, new QTableWidgetItem(QString::number(r.sum, 'f', 2)));
        auto *idItem = new QTableWidgetItem(QString::number(r.id));
        ui->checkWidget->setItem(row, 4, idItem);
    }
    updateTotals();
}

void MainWindow::updateTotals()
{
    double total = 0.0;
    double discount = 0.0;
    if (m_salesManager && m_currentCheckId > 0) {
        const Check ch = m_salesManager->getCheck(m_currentCheckId);
        total = ch.totalAmount;
        discount = ch.discountAmount;
    }
    const double toPay = (total - discount) < 0 ? 0 : (total - discount);
    ui->totalLabel->setText(tr("Итого: %1").arg(QString::number(total, 'f', 2)));
    ui->discountLabel->setText(tr("Скидка: %1").arg(QString::number(discount, 'f', 2)));
    ui->toPayLabel->setText(tr("К оплате: %1").arg(QString::number(toPay, 'f', 2)));
}

void MainWindow::on_newCheckButton_clicked()
{
    if (!m_salesManager || m_employeeId <= 0) return;
    SaleOperationResult r = m_salesManager->createCheck(m_employeeId);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    m_currentCheckId = r.checkId;
    refreshCart();
    ui->statusbar->showMessage(tr("Чек №%1 создан.").arg(m_currentCheckId));
}

void MainWindow::on_findGoodButton_clicked()
{
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала нажмите «Новый чек»."));
        return;
    }
    GoodFind *dlg = WindowCreator::Create<GoodFind>(this, m_core, false);
    if (!dlg || dlg->exec() != QDialog::Accepted)
        return;
    const qint64 qnt = static_cast<qint64>(ui->qtySpinBox->value());
    SaleOperationResult r;
    if (dlg->isBatchSelected()) {
        r = m_salesManager->addSaleByBatchId(m_currentCheckId, dlg->selectedBatchId(), qnt);
    } else {
        r = m_salesManager->addSaleByServiceId(m_currentCheckId, dlg->selectedServiceId(), qnt);
    }
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    refreshCart();
}

void MainWindow::on_removeFromCartButton_clicked()
{
    if (!m_salesManager || m_currentCheckId <= 0) return;
    const int row = ui->checkWidget->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Выберите позицию в чеке."));
        return;
    }
    QTableWidgetItem *idItem = ui->checkWidget->item(row, 4);
    if (!idItem) return;
    const qint64 saleId = idItem->text().toLongLong();
    SaleOperationResult r = m_salesManager->removeSale(saleId);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    refreshCart();
}

void MainWindow::on_discountButton_clicked()
{
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала создайте чек и добавьте позиции."));
        return;
    }
    bool ok = false;
    const double v = QInputDialog::getDouble(this, tr("Скидка"), tr("Сумма скидки (₽):"), 0, 0, 1e9, 2, &ok);
    if (!ok) return;
    SaleOperationResult r = m_salesManager->setCheckDiscount(m_currentCheckId, v);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    updateTotals();
}

void MainWindow::on_payButton_clicked()
{
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Нет активного чека."));
        return;
    }
    const Check ch = m_salesManager->getCheck(m_currentCheckId);
    const double toPay = ch.totalAmount - ch.discountAmount;
    
    // Финализируем чек (списываем товар через StockManager)
    SaleOperationResult result = m_salesManager->finalizeCheck(m_currentCheckId);
    if (!result.success) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Не удалось финализировать чек: %1").arg(result.message));
        return;
    }
    
    QMessageBox::information(this, tr("Оплата"),
        tr("Чек №%1 закрыт.\nК оплате: %2 ₽")
            .arg(m_currentCheckId)
            .arg(QString::number(toPay < 0 ? 0 : toPay, 'f', 2)));
    m_currentCheckId = 0;
    refreshCart();
    ui->statusbar->showMessage(tr("Чек закрыт. Нажмите «Новый чек» для следующей продажи."));
}

void MainWindow::on_actionCategories_triggered()
{
    if (!m_core) return;
    GoodCatsDialog *dlg = WindowCreator::Create<GoodCatsDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionGoods_triggered()
{
    if (!m_core) return;
    GoodsDialog *dlg = WindowCreator::Create<GoodsDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionServices_triggered()
{
    QMessageBox::information(this, tr("Справочники"), tr("Справочник «Услуги» — в разработке."));
}

void MainWindow::on_actionVatRates_triggered()
{
    QMessageBox::information(this, tr("Справочники"), tr("Справочник «Ставки НДС» — в разработке."));
}

