#ifndef EMPLOYEEEDITDIALOG_H
#define EMPLOYEEEDITDIALOG_H

#include <QDialog>
#include <memory>

class EasyPOSCore;
struct Employee;

namespace Ui { class EmployeeEditDialog; }

/**
 * @brief Диалог редактирования сотрудника (ФИО, должность, привязка к пользователю)
 */
class EmployeeEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EmployeeEditDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~EmployeeEditDialog();
    void setData(const Employee &emp);
    QString lastName() const;
    QString firstName() const;
    QString middleName() const;
    int positionId() const;
    int userId() const;
    QString badgeCode() const;

private:
    void fillPositionCombo();
    void fillUserCombo();

    Ui::EmployeeEditDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
};

#endif // EMPLOYEEEDITDIALOG_H
