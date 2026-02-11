#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "easyposcore.h"
#include "RBAC/structures.h"
#include "settings/settingsmanager.h"
#include "sales/salesmanager.h"
#include "sales/stockmanager.h"
#include "sales/structures.h"
#include "ui/sales/goodfind.h"
#include "ui/goodcatsdialog.h"
#include "ui/goodsdialog.h"
#include "ui/servicesdialog.h"
#include "ui/vatratesdialog.h"
#include "ui/positionsdialog.h"
#include "ui/employeesdialog.h"
#include "ui/promotionsdialog.h"
#include "ui/discountdialog.h"
#include "ui/dbsettingsdialog.h"
#include "ui/settingsdialog.h"
#include "ui/checkhistorydialog.h"
#include "ui/alertsdialog.h"
#include "ui/salesreportdialog.h"
#include "ui/reportwizarddialog.h"
#include "ui/recipesdialog.h"
#include "ui/productionrundialog.h"
#include "ui/productionhistorydialog.h"
#include "ui/stockbalancedialog.h"
#include "ui/shiftsdialog.h"
#include "shifts/shiftmanager.h"
#include "shifts/structures.h"
#include "alerts/alertkeys.h"
#include "alerts/alertsmanager.h"
#include "logging/logmanager.h"
#include "ui/paymentdialog.h"
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
#include <QKeyEvent>
#include <QApplication>
#include <QTimer>
#include <QPushButton>
#include <QFile>
#include <QIcon>
#include <QPixmap>

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

    QString appName = m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS");
    QString title = tr("%1 - Касса").arg(appName);
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
    if (m_core) {
        QString logoPath = m_core->getBrandingLogoPath();
        if (!logoPath.isEmpty() && QFile::exists(logoPath)) {
            QIcon icon(logoPath);
            if (!icon.isNull())
                setWindowIcon(icon);
            QPixmap pm(logoPath);
            if (!pm.isNull()) {
                pm = pm.scaled(180, 52, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                ui->brandingLogoLabel->setPixmap(pm);
                ui->brandingLogoLabel->setVisible(true);
            } else {
                ui->brandingLogoLabel->setVisible(false);
            }
        } else {
            ui->brandingLogoLabel->setVisible(false);
        }
        QString appName = m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS");
        ui->brandingAppNameLabel->setText(appName);
    } else {
        ui->brandingAppNameLabel->setText(QStringLiteral("easyPOS"));
    }

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
    applyRoleAccess();
    m_daySummaryLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(m_daySummaryLabel);
    ui->statusbar->addPermanentWidget(new QLabel(tr("Пользователь: %1").arg(m_session.username)));

    ui->checkWidget->insertColumn(4);
    ui->checkWidget->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("НДС")));
    ui->checkWidget->insertColumn(5);
    ui->checkWidget->setColumnHidden(5, true);
    ui->checkWidget->setAlternatingRowColors(true);
    ui->checkWidget->horizontalHeader()->setStretchLastSection(false);
    ui->checkWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->checkWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->checkWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->checkWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->checkWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->checkWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->checkWidget, &QTableWidget::customContextMenuRequested, this, &MainWindow::showCartContextMenu);

    m_barcodeTimer = new QTimer(this);
    m_barcodeTimer->setSingleShot(true);
    connect(m_barcodeTimer, &QTimer::timeout, this, [this]() { m_barcodeBuffer.clear(); });
    qApp->installEventFilter(this);

    connect(ui->actionNewCheck, &QAction::triggered, this, &MainWindow::on_newCheckButton_clicked);
    connect(ui->actionFindGood, &QAction::triggered, this, &MainWindow::on_findGoodButton_clicked);
    connect(ui->actionPay, &QAction::triggered, this, &MainWindow::on_payButton_clicked);

    // Горячие клавиши (F2, F3, F4 — через actions в toolbar)
    new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_R), this, [this] { on_repeatButton_clicked(); });
    new QShortcut(QKeySequence(Qt::Key_Delete), this, [this] { on_removeFromCartButton_clicked(); });

    updateCheckNumberLabel();
    refreshCart();
    updateDaySummary();

    ui->checkWidget->setFocusPolicy(Qt::StrongFocus);
    ui->checkWidget->setFocus();

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

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (m_currentCheckId > 0 && focusWidget() && (focusWidget()->inherits("QSpinBox") || focusWidget()->inherits("QLineEdit")))
        ui->checkWidget->setFocus();
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
}

void MainWindow::applyRoleAccess()
{
    const Role &role = m_session.role;
    const bool isAdmin   = role.hasAccessLevel(AccessLevel::Admin);
    const bool isManager = role.hasAccessLevel(AccessLevel::Manager);
    const bool isCashier = role.hasAccessLevel(AccessLevel::Cashier);

    ui->actionSettings->setVisible(isAdmin);
    ui->actionDbConnection->setVisible(isAdmin);
    ui->actionPrintAfterPay->setVisible(isCashier);

    ui->actionCategories->setVisible(isManager);
    ui->actionGoods->setVisible(isManager);
    ui->actionServices->setVisible(isManager);
    ui->actionVatRates->setVisible(isManager);
    ui->actionPromotions->setVisible(isManager);
    ui->actionPositions->setVisible(isManager);
    ui->actionEmployees->setVisible(isManager);
    ui->actionStockBalance->setVisible(isManager);

    ui->actionRecipes->setVisible(isManager);
    ui->actionProductionRun->setVisible(isManager);
    ui->actionProductionHistory->setVisible(isManager);

    ui->actionCheckHistory->setVisible(isManager);
    ui->actionAlerts->setVisible(isManager);
    ui->actionSalesReport->setVisible(isManager);
    ui->actionSaveCheckToPdf->setVisible(isCashier);

    ui->actionNewCheck->setVisible(isCashier);
    ui->actionFindGood->setVisible(isCashier);
    ui->actionPay->setVisible(isCashier);
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
        const double priceToShow = r.originalUnitPrice > 0.0 ? r.originalUnitPrice : r.unitPrice;
        ui->checkWidget->setItem(row, 2, new QTableWidgetItem(QString::number(priceToShow, 'f', 2)));
        ui->checkWidget->setItem(row, 3, new QTableWidgetItem(QString::number(r.sum, 'f', 2)));
        const QString vatName = m_salesManager->getVatRateName(r.vatRateId);
        ui->checkWidget->setItem(row, 4, new QTableWidgetItem(vatName.isEmpty() ? tr("—") : vatName));
        auto *idItem = new QTableWidgetItem(QString::number(r.id));
        ui->checkWidget->setItem(row, 5, idItem);
    }
    updateTotals();
    updateItemsCountLabel();
}

void MainWindow::updateTotals()
{
    double total = 0.0;
    double discount = 0.0;
    double manualDiscount = 0.0;
    double vatTotal = 0.0;
    if (m_salesManager && m_currentCheckId > 0) {
        const Check ch = m_salesManager->getCheck(m_currentCheckId);
        total = ch.totalAmount;
        manualDiscount = ch.discountAmount;
        const QList<SaleRow> rows = m_salesManager->getSalesByCheck(m_currentCheckId);
        double promotionDiscount = 0.0;
        for (const SaleRow &r : rows) {
            if (r.originalUnitPrice > 0.0 && r.qnt > 0) {
                const double lineFull = r.originalUnitPrice * static_cast<double>(r.qnt);
                if (lineFull > r.sum)
                    promotionDiscount += lineFull - r.sum;
            }
            const double rate = m_salesManager->getVatRatePercent(r.vatRateId);
            if (rate > 0)
                vatTotal += r.sum * rate / (100.0 + rate);
        }
        discount = manualDiscount + promotionDiscount;
    }
    const double toPay = (total - manualDiscount) < 0 ? 0 : (total - manualDiscount);
    ui->totalLabel->setText(tr("Итого: %1 ₽").arg(QString::number(total, 'f', 2)));
    ui->discountLabel->setText(tr("Скидка: %1 ₽").arg(QString::number(discount, 'f', 2)));
    ui->vatTotalLabel->setText(tr("В т.ч. НДС: %1 ₽").arg(QString::number(vatTotal, 'f', 2)));
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

    ShiftManager *shiftMgr = m_core ? m_core->getShiftManager() : nullptr;
    if (!shiftMgr) {
        LOG_WARNING() << "MainWindow::on_newCheckButton_clicked: менеджер смен недоступен";
        QMessageBox::critical(this, tr("Ошибка"), tr("Менеджер смен недоступен."));
        return;
    }
    WorkShift currentShift = shiftMgr->getCurrentShift(m_employeeId);
    if (currentShift.id <= 0) {
        LOG_INFO() << "MainWindow::on_newCheckButton_clicked: отказ — нет активной смены, employeeId=" << m_employeeId;
        QMessageBox msg(this);
        msg.setWindowTitle(tr("Нет активной смены"));
        msg.setText(tr("У вас нет открытой смены. Продажи возможны только во время активной смены.\n\n"
                       "Начните смену в окне «Учёт смен» (Файл → Учёт смен)."));
        msg.setIcon(QMessageBox::Warning);
        QPushButton *openShifts = msg.addButton(tr("Открыть учёт смен"), QMessageBox::ActionRole);
        msg.addButton(QMessageBox::Ok);
        msg.exec();
        if (msg.clickedButton() == openShifts)
            on_actionShifts_triggered();
        return;
    }

    LOG_INFO() << "MainWindow::on_newCheckButton_clicked: создание чека employeeId=" << m_employeeId << "shiftId=" << currentShift.id;
    SaleOperationResult r = m_salesManager->createCheck(m_employeeId, currentShift.id);
    if (!r.success) {
        LOG_WARNING() << "MainWindow::on_newCheckButton_clicked: ошибка создания чека" << r.message;
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    m_currentCheckId = r.checkId;
    LOG_INFO() << "MainWindow::on_newCheckButton_clicked: чек создан checkId=" << m_currentCheckId;
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::CheckCreated,
                    tr("Чек №%1 создан").arg(m_currentCheckId), m_session.userId, m_employeeId);
    updateCheckNumberLabel();
    refreshCart();
    ui->checkWidget->setFocus(); // фокус на таблицу — тогда ввод сканера перехватывается
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
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::SaleAdded,
                    tr("Повтор позиции в чек №%1, кол-во %2").arg(m_currentCheckId).arg(qnt),
                    m_session.userId, m_employeeId);
    refreshCart();
}

// Минимальная и максимальная длина штрихкода (EAN-8 и др.)
static constexpr int BARCODE_MIN_LENGTH = 8;
static constexpr int BARCODE_MAX_LENGTH = 32;

void MainWindow::processBarcode(const QString &barcode)
{
    if (barcode.isEmpty())
        return;
    if (barcode.length() < BARCODE_MIN_LENGTH) {
        QMessageBox::warning(this, tr("Штрихкод"),
            tr("Слишком короткий штрихкод (минимум %1 символов).").arg(BARCODE_MIN_LENGTH));
        return;
    }
    if (barcode.length() > BARCODE_MAX_LENGTH) {
        QMessageBox::warning(this, tr("Штрихкод"),
            tr("Слишком длинный штрихкод (максимум %1 символов).").arg(BARCODE_MAX_LENGTH));
        return;
    }
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала нажмите «Новый чек»."));
        return;
    }
    StockManager *sm = m_core->getStockManager();
    if (!sm) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Недоступен менеджер склада."));
        return;
    }
    const qint64 batchId = sm->getBatchIdByBarcode(barcode);
    if (batchId <= 0) {
        QMessageBox::warning(this, tr("Штрихкод"), tr("Товар с таким штрихкодом не найден. Добавьте товар через «Найти товар»."));
        return;
    }
    const qint64 qnt = static_cast<qint64>(ui->qtySpinBox->value());
    SaleOperationResult r = m_salesManager->addSaleByBatchId(m_currentCheckId, batchId, qnt);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::SaleAdded,
                    tr("Добавлена позиция в чек №%1, batchId=%2, кол-во %3").arg(m_currentCheckId).arg(batchId).arg(qnt),
                    m_session.userId, m_employeeId);
    m_lastBatchId = batchId;
    m_lastServiceId = 0;
    m_lastQnt = qnt;
    refreshCart();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return false;
    QWidget *receiver = qobject_cast<QWidget *>(watched);
    if (!receiver || receiver->window() != this)
        return false;
    QWidget *focus = QApplication::focusWidget();
    if (!focus || focus->window() != this)
        return false;
    const bool focusIsInput = focus->inherits("QLineEdit") || focus->inherits("QSpinBox")
        || focus->inherits("QDoubleSpinBox") || focus->inherits("QComboBox")
        || focus->inherits("QPlainTextEdit") || focus->inherits("QTextEdit");

    QKeyEvent *ke = static_cast<QKeyEvent *>(event);
    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
        if (!m_barcodeBuffer.isEmpty() && m_barcodeBuffer.length() >= BARCODE_MIN_LENGTH) {
            const QString barcode = m_barcodeBuffer.trimmed();
            m_barcodeBuffer.clear();
            m_barcodeTimer->stop();
            processBarcode(barcode);
            if (focusIsInput && focus == ui->qtySpinBox)
                ui->qtySpinBox->setValue(1);
            return true;
        }
        m_barcodeBuffer.clear();
        m_barcodeTimer->stop();
        return false;
    }
    if (ke->key() == Qt::Key_Backspace) {
        if (!m_barcodeBuffer.isEmpty()) {
            m_barcodeBuffer.chop(1);
            m_barcodeTimer->start(400);
            return !focusIsInput;
        }
        return false;
    }
    QString text = ke->text();
    QChar ch;
    if (!text.isEmpty() && text.at(0).isPrint()) {
        ch = text.at(0);
    } else if (ke->key() >= Qt::Key_0 && ke->key() <= Qt::Key_9) {
        ch = QChar(uchar('0' + (ke->key() - Qt::Key_0)));
    } else {
        return false;
    }
    m_barcodeBuffer += ch;
    m_barcodeTimer->start(400);
    return !focusIsInput;
}

void MainWindow::on_findGoodButton_clicked()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_salesManager || m_currentCheckId <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сначала нажмите «Новый чек»."));
        return;
    }
    GoodFind *dlg = WindowCreator::Create<GoodFind>(this, m_core, false);
    if (!dlg || dlg->exec() != QDialog::Accepted)
        return;
    if (!m_core->ensureSessionValid()) { close(); return; }
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
    QTableWidgetItem *idItem = ui->checkWidget->item(row, 5);
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
        else {
            if (auto *alerts = m_core->createAlertsManager())
                alerts->log(AlertCategory::Sales, AlertSignature::SaleRemoved,
                            tr("Удалена позиция saleId=%1 из чека").arg(saleId), m_session.userId, m_employeeId);
            refreshCart();
        }
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
    QTableWidgetItem *idItem = ui->checkWidget->item(row, 5);
    if (!idItem) return;
    const qint64 saleId = idItem->text().toLongLong();
    SaleOperationResult r = m_salesManager->removeSale(saleId);
    if (!r.success) {
        QMessageBox::critical(this, tr("Ошибка"), r.message);
        return;
    }
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::SaleRemoved,
                    tr("Удалена позиция saleId=%1 из чека №%2").arg(saleId).arg(m_currentCheckId),
                    m_session.userId, m_employeeId);
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
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::DiscountApplied,
                    tr("Скидка по чеку №%1: %2 ₽").arg(m_currentCheckId).arg(v, 0, 'f', 2),
                    m_session.userId, m_employeeId);
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

    QString appName = m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS");
    painter.drawText(leftMargin, y, tr("%1 — Чек №%2").arg(appName).arg(checkId));
    y += lineHeight * 2;
    painter.drawText(leftMargin, y, QLocale::system().toString(ch.date, QLocale::ShortFormat) + QLatin1String(" ") + ch.time.toString(Qt::ISODate));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Сотрудник: %1").arg(ch.employeeName));
    y += lineHeight * 2;

    painter.drawText(leftMargin, y, tr("Товар"));
    painter.drawText(leftMargin + 250, y, tr("Кол-во"));
    painter.drawText(leftMargin + 320, y, tr("Цена"));
    painter.drawText(leftMargin + 400, y, tr("Сумма"));
    painter.drawText(leftMargin + 480, y, tr("НДС"));
    y += lineHeight;

    for (const SaleRow &r : rows) {
        const QString vatName = m_salesManager ? m_salesManager->getVatRateName(r.vatRateId) : QString();
        const double priceToShow = r.originalUnitPrice > 0.0 ? r.originalUnitPrice : r.unitPrice;
        painter.drawText(leftMargin, y, r.itemName.left(35));
        painter.drawText(leftMargin + 250, y, QString::number(r.qnt));
        painter.drawText(leftMargin + 320, y, QString::number(priceToShow, 'f', 2));
        painter.drawText(leftMargin + 400, y, QString::number(r.sum, 'f', 2));
        painter.drawText(leftMargin + 480, y, vatName.isEmpty() ? tr("—") : vatName);
        y += lineHeight;
    }
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Итого: %1 ₽").arg(QString::number(ch.totalAmount, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Скидка: %1 ₽").arg(QString::number(ch.discountAmount, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("К оплате: %1 ₽").arg(QString::number(toPay, 'f', 2)));
    QString address = m_core ? m_core->getBrandingAddress() : QString();
    QString legalInfo = m_core ? m_core->getBrandingLegalInfo() : QString();
    if (!address.isEmpty() || !legalInfo.isEmpty()) {
        y += lineHeight * 2;
        if (!address.isEmpty()) {
            painter.drawText(leftMargin, y, address);
            y += lineHeight;
        }
        if (!legalInfo.isEmpty()) {
            for (const QString &line : legalInfo.split(QLatin1Char('\n'))) {
                if (!line.trimmed().isEmpty()) {
                    painter.drawText(leftMargin, y, line.trimmed());
                    y += lineHeight;
                }
            }
        }
    }
}

bool MainWindow::saveCheckToPdf(qint64 checkId)
{
    if (!m_salesManager || checkId <= 0) return false;
    const Check ch = m_salesManager->getCheck(checkId);
    if (ch.id == 0) return false;
    const QList<SaleRow> rows = m_salesManager->getSalesByCheck(checkId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);

    QString appName = m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS");
    QString defaultName = tr("%1_Чек_%2.pdf").arg(appName).arg(checkId);
    QString path = QFileDialog::getSaveFileName(this, tr("Сохранить чек в PDF"),
        defaultName, tr("PDF (*.pdf)"), nullptr, QFileDialog::DontUseNativeDialog);
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
    if (toPay <= 0) {
        QMessageBox::warning(this, tr("Касса"), tr("Сумма к оплате должна быть больше нуля."));
        return;
    }

    QList<PaymentMethodInfo> methods = m_salesManager->getPaymentMethods();
    if (methods.isEmpty()) {
        QMessageBox::warning(this, tr("Оплата"),
            tr("Нет способов оплаты."));
        return;
    }

    PaymentDialog payDlg(this);
    payDlg.setAmountToPay(toPay);
    payDlg.setPaymentMethods(methods);
    if (payDlg.exec() != QDialog::Accepted)
        return;

    const QList<CheckPaymentRow> payments = payDlg.getPayments();
    SaleOperationResult payResult = m_salesManager->recordCheckPayments(m_currentCheckId, payments);
    if (!payResult.success) {
        qWarning() << "MainWindow: recordCheckPayments failed, checkId=" << m_currentCheckId << payResult.message;
        QMessageBox::warning(this, tr("Ошибка"), payResult.message);
        return;
    }
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::PaymentRecorded,
                    tr("Оплата по чеку №%1: %2 ₽").arg(m_currentCheckId).arg(toPay, 0, 'f', 2),
                    m_session.userId, m_employeeId);

    SaleOperationResult result = m_salesManager->finalizeCheck(m_currentCheckId);
    if (!result.success) {
        qWarning() << "MainWindow: finalizeCheck failed, checkId=" << m_currentCheckId << result.message;
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Не удалось финализировать чек: %1").arg(result.message));
        return;
    }

    qInfo() << "MainWindow: check closed, checkId=" << m_currentCheckId << "toPay=" << toPay;
    const qint64 closedCheckId = m_currentCheckId;
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::CheckFinalized,
                    tr("Чек №%1 закрыт, к оплате %2 ₽").arg(closedCheckId).arg(toPay, 0, 'f', 2),
                    m_session.userId, m_employeeId);
    m_currentCheckId = 0;
    updateCheckNumberLabel();
    refreshCart();

    if (m_core && m_core->getSettingsManager() && m_core->getSettingsManager()->boolValue(SettingsKeys::PrintAfterPay, true))
        printCheck(closedCheckId);

    QMessageBox::information(this, tr("Оплата"),
        tr("Чек №%1 закрыт.\nК оплате: %2 ₽")
            .arg(closedCheckId)
            .arg(QString::number(toPay, 'f', 2)));
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
    if (auto *alerts = m_core->createAlertsManager())
        alerts->log(AlertCategory::Sales, AlertSignature::CheckCancelled,
                    tr("Чек №%1 отменён").arg(m_currentCheckId), m_session.userId, m_employeeId);
    m_currentCheckId = 0;
    updateCheckNumberLabel();
    refreshCart();
    ui->statusbar->showMessage(tr("Чек отменён."));
}

void MainWindow::on_actionSaveCheckToPdf_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Cashier)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав."));
        return;
    }
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
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для просмотра истории чеков."));
        return;
    }
    CheckHistoryDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionAlerts_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для просмотра справочника событий."));
        return;
    }
    AlertsDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionShifts_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    ShiftsDialog *dlg = WindowCreator::Create<ShiftsDialog>(this, m_core, false);
    if (dlg) dlg->exec();
}

void MainWindow::on_actionPrintAfterPay_triggered()
{
    // Обрабатывается через connect в конструкторе
}

void MainWindow::on_actionSettings_triggered()
{
    if (!m_core) return;
    if (!m_session.role.hasAccessLevel(AccessLevel::Admin)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав. Только администратор."));
        return;
    }
    qInfo() << "MainWindow: opening Settings dialog";
    SettingsDialog dlg(this, m_core);
    dlg.exec();
    if (auto *sm = m_core->getSettingsManager())
        ui->actionPrintAfterPay->setChecked(sm->boolValue(SettingsKeys::PrintAfterPay, true));
}

void MainWindow::on_actionDbConnection_triggered()
{
    if (!m_core) return;
    if (!m_session.role.hasAccessLevel(AccessLevel::Admin)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав. Только администратор."));
        return;
    }
    DbSettingsDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionAbout_triggered()
{
    QString appName = m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS");
    QMessageBox::about(this, tr("О программе"),
        tr("<h3>%1</h3>"
           "<p>Система автоматизации торговли (касса)</p>"
           "<p>Версия: %2</p>").arg(appName.toHtmlEscaped(), m_core->getProductVersion()));
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

void MainWindow::on_actionReports_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для отчётов."));
        return;
    }
    ReportWizardDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionSalesReport_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для отчётов."));
        return;
    }
    SalesReportDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionRecipes_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для рецептов производства."));
        return;
    }
    RecipesDialog *dlg = WindowCreator::Create<RecipesDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionProductionRun_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для проведения производства."));
        return;
    }
    ProductionRunDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionProductionHistory_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для просмотра истории производств."));
        return;
    }
    ProductionHistoryDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionStockBalance_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для просмотра остатков."));
        return;
    }
    StockBalanceDialog *dlg = WindowCreator::Create<StockBalanceDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionLogout_triggered()
{
    if (QMessageBox::question(this, tr("Выход"), tr("Выйти из учётной записи?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    qInfo() << "MainWindow: user requested logout";
    if (m_core)
        m_core->logout();
}

void MainWindow::on_actionCategories_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочников."));
        return;
    }
    GoodCatsDialog *dlg = WindowCreator::Create<GoodCatsDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionGoods_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочников."));
        return;
    }
    GoodsDialog *dlg = WindowCreator::Create<GoodsDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionServices_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочников."));
        return;
    }
    ServicesDialog *dlg = WindowCreator::Create<ServicesDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionVatRates_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочников."));
        return;
    }
    VatRatesDialog *dlg = WindowCreator::Create<VatRatesDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}

void MainWindow::on_actionPositions_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочника должностей."));
        return;
    }
    PositionsDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionPromotions_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочника маркетинговых акций."));
        return;
    }
    PromotionsDialog dlg(this, m_core);
    dlg.exec();
}

void MainWindow::on_actionEmployees_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочника сотрудников."));
        return;
    }
    EmployeesDialog dlg(this, m_core);
    dlg.exec();
}

