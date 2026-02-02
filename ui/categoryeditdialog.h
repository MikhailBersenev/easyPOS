#ifndef CATEGORYEDITDIALOG_H
#define CATEGORYEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>

class CategoryEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CategoryEditDialog(QWidget *parent = nullptr);
    void setData(const QString &name, const QString &description);
    QString name() const;
    QString description() const;

private:
    QLineEdit *m_nameEdit;
    QPlainTextEdit *m_descriptionEdit;
};

#endif // CATEGORYEDITDIALOG_H
