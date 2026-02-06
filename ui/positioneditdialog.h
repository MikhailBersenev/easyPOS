#ifndef POSITIONEDITDIALOG_H
#define POSITIONEDITDIALOG_H

#include <QDialog>

namespace Ui { class PositionEditDialog; }

class PositionEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PositionEditDialog(QWidget *parent = nullptr);
    ~PositionEditDialog();
    void setData(const QString &name, bool isActive);
    QString name() const;
    bool isActive() const;

private:
    Ui::PositionEditDialog *ui;
};

#endif // POSITIONEDITDIALOG_H
