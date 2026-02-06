#ifndef PROMOTIONEDITDIALOG_H
#define PROMOTIONEDITDIALOG_H

#include <QDialog>

namespace Ui { class PromotionEditDialog; }

class PromotionEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PromotionEditDialog(QWidget *parent = nullptr);
    ~PromotionEditDialog();
    void setData(const QString &name, const QString &description, double percent, double sum);
    QString name() const;
    QString description() const;
    double percent() const;
    double sum() const;

private:
    Ui::PromotionEditDialog *ui;
};

#endif // PROMOTIONEDITDIALOG_H
