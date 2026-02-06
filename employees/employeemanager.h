#ifndef EMPLOYEEMANAGER_H
#define EMPLOYEEMANAGER_H

#include <QObject>
#include <QList>
#include <QSqlError>
#include "structures.h"

class DatabaseConnection;

class EmployeeManager : public QObject
{
    Q_OBJECT
public:
    explicit EmployeeManager(QObject *parent = nullptr);
    void setDatabaseConnection(DatabaseConnection *conn);

    QList<Employee> list(bool includeDeleted = false);
    Employee getEmployee(int id, bool includeDeleted = true);
    bool add(Employee &emp);
    bool update(const Employee &emp);
    bool setDeleted(int id, bool deleted = true);

    QSqlError lastError() const { return m_lastError; }

private:
    DatabaseConnection *m_db = nullptr;
    mutable QSqlError m_lastError;
};

#endif // EMPLOYEEMANAGER_H
