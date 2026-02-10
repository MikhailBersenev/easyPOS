#ifndef PAYMENTDIALOG_H
#define PAYMENTDIALOG_H

#include <QDialog>
#include "../sales/structures.h"

namespace Ui { class PaymentDialog; }

class PaymentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaymentDialog(QWidget *parent = nullptr);
    ~PaymentDialog();

    void setAmountToPay(double amount);
    void setPaymentMethods(const QList<PaymentMethodInfo> &methods);
    QList<CheckPaymentRow> getPayments() const;

private slots:
    void onFullAmount();
    void updateTotal();

private:
    void setupConnections();
    double sumPayments() const;
    void updateOkButton();

    Ui::PaymentDialog *ui;
    double m_amountToPay = 0.0;
    QList<PaymentMethodInfo> m_methods;
};

#endif // PAYMENTDIALOG_H
