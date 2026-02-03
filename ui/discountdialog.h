#ifndef DISCOUNTDIALOG_H
#define DISCOUNTDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QDoubleSpinBox>

class DiscountDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiscountDialog(double totalAmount, QWidget *parent = nullptr);
    double discountAmount() const;

private:
    QRadioButton *m_sumRadio;
    QRadioButton *m_percentRadio;
    QDoubleSpinBox *m_sumSpin;
    QDoubleSpinBox *m_percentSpin;
    double m_totalAmount;
    void updateFromPercent();
    void updateFromSum();
};

#endif // DISCOUNTDIALOG_H
