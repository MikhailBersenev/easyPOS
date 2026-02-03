#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QCloseEvent>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EasyPOSCore;
#include "RBAC/structures.h"
#include "sales/structures.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> core, const UserSession &session);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_newCheckButton_clicked();
    void on_cancelCheckButton_clicked();
    void on_repeatButton_clicked();
    void on_findGoodButton_clicked();
    void on_removeFromCartButton_clicked();
    void on_discountButton_clicked();
    void on_printButton_clicked();
    void on_payButton_clicked();
    void on_actionCategories_triggered();
    void on_actionGoods_triggered();
    void on_actionServices_triggered();
    void on_actionVatRates_triggered();
    void on_actionLogout_triggered();
    void on_actionSaveCheckToPdf_triggered();
    void on_actionCheckHistory_triggered();
    void on_actionPrintAfterPay_triggered();
    void on_actionDbConnection_triggered();
    void on_actionAbout_triggered();
    void on_actionShortcuts_triggered();
    void on_actionSalesReport_triggered();

private:
    void refreshCart();
    void updateTotals();
    void updateCheckNumberLabel();
    void updateItemsCountLabel();
    void updateDaySummary();
    void setPosEnabled(bool enabled);
    void printCheck(qint64 checkId);
    bool saveCheckToPdf(qint64 checkId);
    void renderCheckContent(QPainter &painter, qint64 checkId, const Check &ch,
                           const QList<SaleRow> &rows, double toPay);
    void showCartContextMenu(const QPoint &pos);

    Ui::MainWindow *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    UserSession m_session;
    class SalesManager *m_salesManager = nullptr;
    qint64 m_employeeId = 0;
    qint64 m_currentCheckId = 0;
    QLabel *m_daySummaryLabel = nullptr;
    qint64 m_lastBatchId = 0;
    qint64 m_lastServiceId = 0;
    qint64 m_lastQnt = 1;
};

#endif // MAINWINDOW_H
