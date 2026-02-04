#ifndef CATEGORYEDITDIALOG_H
#define CATEGORYEDITDIALOG_H

#include <QDialog>

namespace Ui { class CategoryEditDialog; }

class CategoryEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CategoryEditDialog(QWidget *parent = nullptr);
    ~CategoryEditDialog();
    void setData(const QString &name, const QString &description);
    QString name() const;
    QString description() const;

private:
    Ui::CategoryEditDialog *ui;
};

#endif // CATEGORYEDITDIALOG_H
