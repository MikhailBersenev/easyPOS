#ifndef GOODEDITDIALOG_H
#define GOODEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QCheckBox>

class GoodEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoodEditDialog(QWidget *parent = nullptr);
    void setCategories(const QList<int> &ids, const QStringList &names);
    void setData(const QString &name, const QString &description, int categoryId, bool isActive);
    QString name() const;
    QString description() const;
    int categoryId() const;
    bool isActive() const;

private:
    QLineEdit *m_nameEdit;
    QPlainTextEdit *m_descriptionEdit;
    QComboBox *m_categoryCombo;
    QCheckBox *m_activeCheck;
    QList<int> m_categoryIds;
};

#endif // GOODEDITDIALOG_H
