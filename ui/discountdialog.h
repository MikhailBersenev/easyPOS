#ifndef DISCOUNTDIALOG_H
#define DISCOUNTDIALOG_H

#include <QDialog>

namespace Ui { class DiscountDialog; }

class DiscountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiscountDialog(double totalAmount, QWidget *parent = nullptr);
    ~DiscountDialog();
    double discountAmount() const;

private slots:
    void updateFromPercent();
    void updateFromSum();

private:
    Ui::DiscountDialog *ui;
    double m_totalAmount;
};

#endif // DISCOUNTDIALOG_H
