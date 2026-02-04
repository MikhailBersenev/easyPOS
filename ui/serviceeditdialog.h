#ifndef SERVICEEDITDIALOG_H
#define SERVICEEDITDIALOG_H

#include <QDialog>

namespace Ui { class ServiceEditDialog; }

class ServiceEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceEditDialog(QWidget *parent = nullptr);
    ~ServiceEditDialog();
    void setCategories(const QList<int> &ids, const QStringList &names);
    void setData(const QString &name, const QString &description, double price, int categoryId);
    QString name() const;
    QString description() const;
    double price() const;
    int categoryId() const;

private:
    Ui::ServiceEditDialog *ui;
    QList<int> m_categoryIds;
};

#endif // SERVICEEDITDIALOG_H
