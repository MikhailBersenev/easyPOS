#ifndef VATRATEEDITDIALOG_H
#define VATRATEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>

class VatRateEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VatRateEditDialog(QWidget *parent = nullptr);
    void setData(const QString &name, double rate);
    QString name() const;
    double rate() const;

private:
    QLineEdit *m_nameEdit;
    QDoubleSpinBox *m_rateSpin;
};

#endif // VATRATEEDITDIALOG_H
