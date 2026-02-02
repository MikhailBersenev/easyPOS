#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EasyPOSCore;
#include "RBAC/structures.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> core, const UserSession &session);
    ~MainWindow();

private slots:
    void on_newCheckButton_clicked();
    void on_findGoodButton_clicked();
    void on_removeFromCartButton_clicked();
    void on_discountButton_clicked();
    void on_payButton_clicked();
    void on_actionCategories_triggered();
    void on_actionGoods_triggered();
    void on_actionServices_triggered();
    void on_actionVatRates_triggered();

private:
    void refreshCart();
    void updateTotals();
    void setPosEnabled(bool enabled);

    Ui::MainWindow *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    UserSession m_session;
    class SalesManager *m_salesManager = nullptr;
    qint64 m_employeeId = 0;
    qint64 m_currentCheckId = 0;
};

#endif // MAINWINDOW_H
