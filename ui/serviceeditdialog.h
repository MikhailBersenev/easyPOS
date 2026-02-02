#ifndef SERVICEEDITDIALOG_H
#define SERVICEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDoubleSpinBox>
#include <QComboBox>

class ServiceEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceEditDialog(QWidget *parent = nullptr);
    void setCategories(const QList<int> &ids, const QStringList &names);
    void setData(const QString &name, const QString &description, double price, int categoryId);
    QString name() const;
    QString description() const;
    double price() const;
    int categoryId() const;

private:
    QLineEdit *m_nameEdit;
    QPlainTextEdit *m_descriptionEdit;
    QDoubleSpinBox *m_priceSpin;
    QComboBox *m_categoryCombo;
    QList<int> m_categoryIds;
};

#endif // SERVICEEDITDIALOG_H
