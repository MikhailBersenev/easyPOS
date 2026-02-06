#ifndef EMPLOYEESDIALOG_H
#define EMPLOYEESDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;
struct Employee;

/**
 * @brief Справочник сотрудников
 */
class EmployeesDialog : public ReferenceDialog
{
    Q_OBJECT
public:
    explicit EmployeesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~EmployeesDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditEmployee(const Employee *existing = nullptr);
};

#endif // EMPLOYEESDIALOG_H
