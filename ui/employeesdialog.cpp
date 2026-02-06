#include "employeesdialog.h"
#include "employeeeditdialog.h"
#include "../easyposcore.h"
#include "../employees/employeemanager.h"
#include "../employees/structures.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include "../RBAC/accountmanager.h"
#include "../RBAC/structures.h"

EmployeesDialog::EmployeesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Сотрудники"))
{
    setupColumns({tr("ID"), tr("Фамилия"), tr("Имя"), tr("Отчество"), tr("Должность"), tr("Пользователь")},
                 {50, 120, 100, 120, 150, 120});
    loadTable();
}

EmployeesDialog::~EmployeesDialog()
{
}

void EmployeesDialog::loadTable(bool includeDeleted)
{
    clearTable();
    if (!m_core || !m_core->getDatabaseConnection())
        return;

    EmployeeManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<Employee> list = mgr.list(includeDeleted);

    AccountManager *am = m_core->createAccountManager(this);
    for (const Employee &e : list) {
        const int row = insertRow();
        setCellText(row, 0, QString::number(e.id));
        setCellText(row, 1, e.lastName);
        setCellText(row, 2, e.firstName);
        setCellText(row, 3, e.middleName);
        setCellText(row, 4, e.position.name);
        QString userName;
        if (e.userId > 0 && am) {
            User u = am->getUser(e.userId, false);
            userName = u.name;
        }
        setCellText(row, 5, userName);
    }
    updateRecordCount();
}

void EmployeesDialog::addRecord()
{
    addOrEditEmployee(nullptr);
}

void EmployeesDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;
    const int id = cellText(row, 0).toInt();
    EmployeeManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    Employee emp = mgr.getEmployee(id, true);
    if (emp.id == 0)
        return;
    addOrEditEmployee(&emp);
}

void EmployeesDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;
    const int id = cellText(row, 0).toInt();
    const QString name = tr("%1 %2 %3").arg(cellText(row, 1), cellText(row, 2), cellText(row, 3)).trimmed();
    if (!askConfirmation(tr("Пометить сотрудника «%1» как удалённого?").arg(name)))
        return;

    EmployeeManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    if (!mgr.setDeleted(id, true)) {
        showError(tr("Не удалось удалить: %1").arg(mgr.lastError().text()));
        return;
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
        alerts->log(AlertCategory::Reference, AlertSignature::EmployeeDeleted,
                   tr("Сотрудник помечен удалённым: %1 (id=%2)").arg(name).arg(id), uid, empId);
    }
    showInfo(tr("Сотрудник помечен как удалённый."));
    loadTable();
}

void EmployeesDialog::addOrEditEmployee(const Employee *existing)
{
    EmployeeEditDialog dlg(this, m_core);
    if (existing) {
        dlg.setWindowTitle(tr("Редактирование сотрудника"));
        dlg.setData(*existing);
    } else {
        dlg.setWindowTitle(tr("Новый сотрудник"));
    }
    if (dlg.exec() != QDialog::Accepted)
        return;

    EmployeeManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());

    if (existing) {
        Employee emp = *existing;
        emp.lastName = dlg.lastName();
        emp.firstName = dlg.firstName();
        emp.middleName = dlg.middleName();
        emp.position.id = dlg.positionId();
        emp.userId = dlg.userId();
        emp.badgeCode = dlg.badgeCode();
        if (!mgr.update(emp)) {
            showError(tr("Не удалось сохранить: %1").arg(mgr.lastError().text()));
            return;
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::EmployeeUpdated,
                       tr("Сотрудник обновлён: %1 %2 %3 (id=%4)").arg(emp.lastName, emp.firstName, emp.middleName).arg(emp.id), uid, empId);
        }
        showInfo(tr("Сотрудник обновлён."));
    } else {
        Employee emp;
        emp.lastName = dlg.lastName();
        emp.firstName = dlg.firstName();
        emp.middleName = dlg.middleName();
        emp.position.id = dlg.positionId();
        emp.userId = dlg.userId();
        emp.badgeCode = dlg.badgeCode();
        if (!mgr.add(emp)) {
            showError(tr("Не удалось добавить: %1").arg(mgr.lastError().text()));
            return;
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::EmployeeCreated,
                       tr("Сотрудник добавлен: %1 %2 %3 (id=%4)").arg(emp.lastName, emp.firstName, emp.middleName).arg(emp.id), uid, empId);
        }
        showInfo(tr("Сотрудник добавлен."));
    }
    loadTable();
}
