#ifndef VATRATEEDITDIALOG_H
#define VATRATEEDITDIALOG_H

#include <QDialog>

namespace Ui { class VatRateEditDialog; }

class VatRateEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VatRateEditDialog(QWidget *parent = nullptr);
    ~VatRateEditDialog();
    void setData(const QString &name, double rate);
    QString name() const;
    double rate() const;

private:
    Ui::VatRateEditDialog *ui;
};

#endif // VATRATEEDITDIALOG_H
