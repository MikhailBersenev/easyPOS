#ifndef SHIFTSDIALOG_H
#define SHIFTSDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui { class ShiftsDialog; }
class EasyPOSCore;

class ShiftsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShiftsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~ShiftsDialog();

private slots:
    void on_refreshButton_clicked();
    void on_startShiftButton_clicked();
    void on_endShiftButton_clicked();

private:
    void loadShifts();
    void updateCurrentShiftLabel();

    Ui::ShiftsDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    qint64 m_employeeId = 0;
};

#endif // SHIFTSDIALOG_H
