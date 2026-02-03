#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "easyposcore.h"
#include "RBAC/structures.h"
#include "settings/settingsmanager.h"
#include "sales/salesmanager.h"
#include "sales/structures.h"
#include "ui/sales/goodfind.h"
#include "ui/goodcatsdialog.h"
#include "ui/goodsdialog.h"
#include "ui/servicesdialog.h"
#include "ui/vatratesdialog.h"
#include "ui/discountdialog.h"
#include "ui/dbsettingsdialog.h"
#include "ui/checkhistorydialog.h"
#include "ui/salesreportdialog.h"
#include "ui/windowcreator.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QShortcut>
#include <QKeySequence>
#include <QDate>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrinterInfo>
#include <QPainter>
#include <QFileDialog>
#include <QLocale>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> core, const UserSession &session)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_core(core)
    , m_session(session)
{
    ui->setupUi(this);
    connect(m_core.get(), &EasyPOSCore::sessionInvalidated, this, &MainWindow::close);

    if (auto *sm = m_core->getSettingsManager()) {
        ui->actionPrintAfterPay->setChecked(sm->boolValue(SettingsKeys::PrintAfterPay, true));
    }
    connect(ui->actionPrintAfterPay, &QAction::toggled, this, [this](bool checked) {
        if (auto *sm = m_core->getSettingsManager()) {
            sm->setValue(SettingsKeys::PrintAfterPay, checked);
            sm->sync();
        }
    });

    QString title = tr("easyPOS - Касса");
    if (m_core) {
        QString host, user;
        if (auto *sm = m_core->getSettingsManager()) {
            host = sm->stringValue(SettingsKeys::DbHost, QStringLiteral("localhost"));
            if (host.isEmpty()) host = QStringLiteral("localhost");
        }
        user = m_session.username;
        if (!host.isEmpty() || !user.isEmpty()) {
            title += QLatin1String(" | ");
            if (!host.isEmpty()) title += host;
            if (!host.isEmpty() && !user.isEmpty()) title += QLatin1String(" | ");
            if (!user.isEmpty()) title += user;
        }
    }
    setWindowTitle(title);

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
    m_daySummaryLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(m_daySummaryLabel);
    ui->statusbar->addPermanentWidget(new QLabel(tr("Пользователь: %1").arg(m_session.username)));

    ui->checkWidget->insertColumn(4);
    ui->checkWidget->setColumnHidden(4, true);
    ui->checkWidget->setAlternatingRowColors(true);
    ui->checkWidget->horizontalHeader()->setStretchLastSection(false);
    ui->checkWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->checkWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->checkWidget, &QTableWidget::customContextMenuRequested, this, &MainWindow::showCartContextMenu);

    connect(ui->searchLineEdit, &QLineEdit::returnPressed, this, &MainWindow::on_findGoodButton_clicked);
    connect(ui->actionNewCheck, &QAction::triggered, this, &MainWindow::on_newCheckButton_clicked);
    connect(ui->actionFindGood, &QAction::triggered, this, &MainWindow::on_findGoodButton_clicked);
    connect(ui->actionPay, &QAction::triggered, this, &MainWindow::on_payButton_clicked);

    // Горячие клавиши (F2, F3, F4 — через actions в toolbar)
    new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_R), this, [this] { on_repeatButton_clicked(); });
    new QShortcut(QKeySequence(Qt::Key_Delete), this, [this] { on_removeFromCartButton_clicked(); });

    updateCheckNumberLabel();
    refreshCart();
    updateDaySummary();

    if (auto *sm = m_core->getSettingsManager()) {
        const QByteArray geo = sm->value(SettingsKeys::MainWindowGeometry).toByteArray();
        if (!geo.isEmpty())
            restoreGeometry(geo);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_core && m_core->getSettingsManager()) {
        m_core->getSettingsManager()->setValue(SettingsKeys::MainWindowGeometry, saveGeometry());
        m_core->getSettingsManager()->sync();
    }
    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setPosEnabled(bool enabled)
{
    ui->newCheckButton->setEnabled(enabled);
    ui->cancelCheckButton->setEnabled(enabled);
    ui->repeatButton->setEnabled(enabled);
    ui->findGoodButton->setEnabled(enabled);
    ui->removeFromCartButton->setEnabled(enabled);
    ui->discountButton->setEnabled(enabled);
    ui->printButton->setEnabled(enabled);
    ui->payButton->setEnabled(enabled);
    ui->checkWidget->setEnabled(enabled);
    ui->qtySpinBox->setEnabled(enabled);
    ui->searchLineEdit->setEnabled(enabled);
}

void MainWindow::refreshCart()
{
    ui->checkWidget->setRowCount(0);
    if (!m_salesManager || m_currentCheckId <= 0) {
        updateTotals();
        updateItemsCountLabel();
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
    updateItemsCountLabel();
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
    ui->totalLabel->setText(tr("Итого: %1 ₽").arg(QString::number(total, 'f', 2)));
    ui->discountLabel->setText(tr("Скидка: %1 ₽").arg(QString::number(discount, 'f', 2)));
    ui->toPayLabel->setText(tr("К оплате: %1 ₽").arg(QString::number(toPay, 'f', 2)));
}

void MainWindow::updateCheckNumberLabel()
{
    if (m_currentCheckId > 0)
        ui->checkNumberLabel->setText(tr(" №%1").arg(m_currentCheckId));
    else
        ui->checkNumberLabel->setText(QString());
}

void MainWindow::updateItemsCountLabel()
{
    const int count = ui->checkWidget->rowCount();
    if (m_currentCheckId > 0 && count > 0)
        ui->itemsCountLabel->setText(tr(" (%1 поз.)").arg(count));
    else
        ui->itemsCountLabel->setText(QString());
}

void MainWindow::updateDaySummary()
{
    if (!m_daySummaryLabel) return;
    if (!m_salesManager) {
        m_daySummaryLabel->setText(QString());
        return;
    }
    const QDate today = QDate::currentDate();
    const QList<Check> checks = m_salesManager->getChecks(today, today, -1, false);
    double dayTotal = 0;
    for (const Check &ch : checks) {
        dayTotal += (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);
    }
    m_daySummaryLabel->setText(tr("Сегодня: %1 чеков, %2 ₽").arg(checks.size()).arg(QString::number(dayTotal, 'f', 2)));
}

void MainWindow::on_newCheckButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_employeeId <= 0) return;
    SaleOperationResult r = m_salesManager->createCheck(m_employeeId);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    m_currentCheckId = r.checkId;
    updateCheckNumberLabel();
    refreshCart();
    ui->statusbar->showMessage(tr("Чек №%1 создан.").arg(m_currentCheckId));
}

void MainWindow::on_repeatButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала нажмите «Новый чек»."));
        return;
    }
    if (m_lastBatchId <= 0 && m_lastServiceId <= 0) {
        QMessageBox::information(this, tr("Касса"), tr("Нет последней позиции для повтора."));
        return;
    }
    const qint64 qnt = static_cast<qint64>(ui->qtySpinBox->value());
    SaleOperationResult r;
    if (m_lastBatchId > 0) {
        r = m_salesManager->addSaleByBatchId(m_currentCheckId, m_lastBatchId, qnt);
    } else {
        r = m_salesManager->addSaleByServiceId(m_currentCheckId, m_lastServiceId, qnt);
    }
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    refreshCart();
}

void MainWindow::on_findGoodButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала нажмите «Новый чек»."));
        return;
    }
    GoodFind *dlg = WindowCreator::Create<GoodFind>(this, m_core, false);
    if (dlg && !ui->searchLineEdit->text().trimmed().isEmpty())
        dlg->setInitialSearch(ui->searchLineEdit->text().trimmed());
    if (!dlg || dlg->exec() != QDialog::Accepted)
        return;
    if (!m_core->ensureSessionValid()) { close(); return; }
    ui->searchLineEdit->clear();
    const qint64 qnt = static_cast<qint64>(ui->qtySpinBox->value());
    SaleOperationResult r;
    if (dlg->isBatchSelected()) {
        m_lastBatchId = dlg->selectedBatchId();
        m_lastServiceId = 0;
        m_lastQnt = qnt;
        r = m_salesManager->addSaleByBatchId(m_currentCheckId, m_lastBatchId, qnt);
    } else {
        m_lastBatchId = 0;
        m_lastServiceId = dlg->selectedServiceId();
        m_lastQnt = qnt;
        r = m_salesManager->addSaleByServiceId(m_currentCheckId, m_lastServiceId, qnt);
    }
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    refreshCart();
}

void MainWindow::showCartContextMenu(const QPoint &pos)
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) return;
    const int row = ui->checkWidget->indexAt(pos).row();
    if (row < 0) return;
    QTableWidgetItem *idItem = ui->checkWidget->item(row, 4);
    if (!idItem) return;
    const qint64 saleId = idItem->text().toLongLong();

    QMenu menu(this);
    QAction *actRemove = menu.addAction(tr("Удалить (Del)"));
    QAction *actQty = menu.addAction(tr("Изменить количество..."));
    QAction *chosen = menu.exec(ui->checkWidget->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == actRemove) {
        SaleOperationResult r = m_salesManager->removeSale(saleId);
        if (!r.success)
            QMessageBox::critical(this, tr("Ошибка"), r.message);
        else
            refreshCart();
        return;
    }
    if (chosen == actQty) {
        SaleRow sr = m_salesManager->getSale(saleId);
        if (sr.id == 0) return;
        bool ok;
        int newQnt = QInputDialog::getInt(this, tr("Количество"),
            tr("Новое количество для «%1»:").arg(sr.itemName),
            static_cast<int>(sr.qnt), 1, 99999, 1, &ok);
        if (!ok || static_cast<qint64>(newQnt) == sr.qnt) return;
        SaleOperationResult r = m_salesManager->updateSaleQuantity(saleId, static_cast<qint64>(newQnt));
        if (!r.success)
            QMessageBox::critical(this, tr("Ошибка"), r.message);
        else
            refreshCart();
    }
}

void MainWindow::on_removeFromCartButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
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
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала создайте чек и добавьте позиции."));
        return;
    }
    const Check ch = m_salesManager->getCheck(m_currentCheckId);
    const double total = ch.totalAmount;
    DiscountDialog dlg(total, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    const double v = dlg.discountAmount();
    SaleOperationResult r = m_salesManager->setCheckDiscount(m_currentCheckId, v);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    updateTotals();
}

void MainWindow::on_printButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Печать"), tr("Нет активного чека для печати."));
        return;
    }
    printCheck(m_currentCheckId);
}

void MainWindow::renderCheckContent(QPainter &painter, qint64 checkId, const Check &ch,
                                    const QList<SaleRow> &rows, double toPay)
{
    const int lineHeight = 24;
    int y = 50;
    const int leftMargin = 50;

    painter.drawText(leftMargin, y, tr("easyPOS — Чек №%1").arg(checkId));
    y += lineHeight * 2;
    painter.drawText(leftMargin, y, QLocale::system().toString(ch.date, QLocale::ShortFormat) + QLatin1String(" ") + ch.time.toString(Qt::ISODate));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Сотрудник: %1").arg(ch.employeeName));
    y += lineHeight * 2;

    painter.drawText(leftMargin, y, tr("Товар"));
    painter.drawText(leftMargin + 250, y, tr("Кол-во"));
    painter.drawText(leftMargin + 320, y, tr("Цена"));
    painter.drawText(leftMargin + 400, y, tr("Сумма"));
    y += lineHeight;

    for (const SaleRow &r : rows) {
        painter.drawText(leftMargin, y, r.itemName.left(35));
        painter.drawText(leftMargin + 250, y, QString::number(r.qnt));
        painter.drawText(leftMargin + 320, y, QString::number(r.unitPrice, 'f', 2));
        painter.drawText(leftMargin + 400, y, QString::number(r.sum, 'f', 2));
        y += lineHeight;
    }
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Итого: %1 ₽").arg(QString::number(ch.totalAmount, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Скидка: %1 ₽").arg(QString::number(ch.discountAmount, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("К оплате: %1 ₽").arg(QString::number(toPay, 'f', 2)));
}

bool MainWindow::saveCheckToPdf(qint64 checkId)
{
    if (!m_salesManager || checkId <= 0) return false;
    const Check ch = m_salesManager->getCheck(checkId);
    if (ch.id == 0) return false;
    const QList<SaleRow> rows = m_salesManager->getSalesByCheck(checkId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);

    QString defaultName = tr("Чек_%1.pdf").arg(checkId);
    QString path = QFileDialog::getSaveFileName(this, tr("Сохранить чек в PDF"),
        defaultName, tr("PDF (*.pdf)"));
    if (path.isEmpty()) return false;
    if (!path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive))
        path += QLatin1String(".pdf");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, tr("PDF"), tr("Не удалось создать PDF."));
        return false;
    }
    renderCheckContent(painter, checkId, ch, rows, toPay);
    painter.end();
    return true;
}

void MainWindow::printCheck(qint64 checkId)
{
    if (!m_salesManager || checkId <= 0) return;
    const Check ch = m_salesManager->getCheck(checkId);
    if (ch.id == 0) return;
    const QList<SaleRow> rows = m_salesManager->getSalesByCheck(checkId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);

    const bool hasPrinters = !QPrinterInfo::availablePrinters().isEmpty() || !QPrinterInfo::defaultPrinter().isNull();

    if (!hasPrinters) {
        if (saveCheckToPdf(checkId))
            QMessageBox::information(this, tr("Печать"), tr("Чек сохранён в PDF."));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() != QDialog::Accepted) return;

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, tr("Печать"), tr("Не удалось начать печать."));
        return;
    }
    renderCheckContent(painter, checkId, ch, rows, toPay);
    painter.end();
}

void MainWindow::on_payButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Нет активного чека."));
        return;
    }
    const Check ch = m_salesManager->getCheck(m_currentCheckId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);
    if (QMessageBox::question(this, tr("Оплата"),
            tr("Подтвердить оплату %1 ₽?\nЧек №%2")
                .arg(QString::number(toPay, 'f', 2))
                .arg(m_currentCheckId),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    // Финализируем чек (списываем товар через StockManager)
    SaleOperationResult result = m_salesManager->finalizeCheck(m_currentCheckId);
    if (!result.success) {
        QMessageBox::warning(this, tr("Ошибка"), 
            tr("Не удалось финализировать чек: %1").arg(result.message));
        return;
    }

    const qint64 closedCheckId = m_currentCheckId;
    m_currentCheckId = 0;
    updateCheckNumberLabel();
    refreshCart();

    if (m_core && m_core->getSettingsManager() && m_core->getSettingsManager()->boolValue(SettingsKeys::PrintAfterPay, true))
        printCheck(closedCheckId);

    QMessageBox::information(this, tr("Оплата"),
        tr("Чек №%1 закрыт.\nК оплате: %2 ₽")
            .arg(closedCheckId)
            .arg(QString::number(toPay < 0 ? 0 : toPay, 'f', 2)));
    updateDaySummary();
    ui->statusbar->showMessage(tr("Чек закрыт. Нажмите «Новый чек» для следующей продажи."));
}

void MainWindow::on_cancelCheckButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Нет активного чека."));
        return;
    }
    if (QMessageBox::question(this, tr("Отмена чека"),
            tr("Отменить чек №%1? Все позиции будут удалены.").arg(m_currentCheckId),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    SaleOperationResult r = m_salesManager->cancelCheck(m_currentCheckId);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    m_currentCheckId = 0;
    updateCheckNumberLabel();
    refreshCart();
    ui->statusbar->showMessage(tr("Чек отменён."));
}

void MainWindow::on_actionSaveCheckToPdf_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("PDF"), tr("Нет активного чека для сохранения."));
        return;
    }
    if (saveCheckToPdf(m_currentCheckId))
        QMessageBox::information(this, tr("PDF"), tr("Чек сохранён в PDF."));
}

void MainWindow::on_actionCheckHistory_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    CheckHistoryDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionPrintAfterPay_triggered()
{
    // Обрабатывается через connect в конструкторе
}

void MainWindow::on_actionDbConnection_triggered()
{
    if (!m_core) return;
    DbSettingsDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("О программе"),
        tr("<h3>easyPOS</h3>"
           "<p>Система автоматизации торговли (касса)</p>"
           "<p>Версия: 1.0</p>"));
}

void MainWindow::on_actionShortcuts_triggered()
{
    QMessageBox::information(this, tr("Горячие клавиши"),
        tr("F2 — Новый чек<br>"
           "F3 — Поиск товара<br>"
           "Ctrl+R — Повторить последнюю позицию<br>"
           "F4 — Оплатить<br>"
           "Delete — Удалить позицию из чека<br>"
           "Ctrl+Q — Выход<br>"
           "F1 — Справка по горячим клавишам"));
}

void MainWindow::on_actionSalesReport_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    SalesReportDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionLogout_triggered()
{
    if (QMessageBox::question(this, tr("Выход"), tr("Выйти из учётной записи?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    if (m_core)
        m_core->logout();
}

void MainWindow::on_actionCategories_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    GoodCatsDialog *dlg = WindowCreator::Create<GoodCatsDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionGoods_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    GoodsDialog *dlg = WindowCreator::Create<GoodsDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionServices_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    ServicesDialog *dlg = WindowCreator::Create<ServicesDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionVatRates_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    VatRatesDialog *dlg = WindowCreator::Create<VatRatesDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

