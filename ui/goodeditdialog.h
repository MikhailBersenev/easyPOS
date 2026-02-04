#ifndef GOODEDITDIALOG_H
#define GOODEDITDIALOG_H

#include <QDialog>

namespace Ui { class GoodEditDialog; }

class GoodEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoodEditDialog(QWidget *parent = nullptr);
    ~GoodEditDialog();
    void setCategories(const QList<int> &ids, const QStringList &names);
    void setData(const QString &name, const QString &description, int categoryId, bool isActive);
    QString name() const;
    QString description() const;
    int categoryId() const;
    bool isActive() const;

private:
    Ui::GoodEditDialog *ui;
    QList<int> m_categoryIds;
};

#endif // GOODEDITDIALOG_H
