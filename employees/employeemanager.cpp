#include "employeemanager.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

EmployeeManager::EmployeeManager(QObject *parent) : QObject(parent), m_db(nullptr) {}

static QString escapeSql(const QString &s)
{
    return QString(s).replace(QLatin1Char('\''), QLatin1String("''"));
}

void EmployeeManager::setDatabaseConnection(DatabaseConnection *conn)
{
    m_db = conn;
}

QList<Employee> EmployeeManager::list(bool includeDeleted)
{
    QList<Employee> result;
    if (!m_db || !m_db->isConnected())
        return result;

    QSqlQuery q(m_db->getDatabase());
    QString sql = QStringLiteral(
        "SELECT e.id, e.lastname, e.firstname, e.middlename, e.positionid, e.userid, e.badgecode, e.updatedate, e.isdeleted, "
        "p.name AS positionname "
        "FROM employees e "
        "LEFT JOIN positions p ON p.id = e.positionid ");
    if (!includeDeleted)
        sql += QStringLiteral("WHERE (e.isdeleted IS NULL OR e.isdeleted = false) ");
    sql += QStringLiteral("ORDER BY e.lastname, e.firstname");
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return result;
    }
    while (q.next()) {
        Employee emp;
        emp.id = q.value(0).toInt();
        emp.lastName = q.value(1).toString();
        emp.firstName = q.value(2).toString();
        emp.middleName = q.value(3).toString();
        emp.position.id = q.value(4).toInt();
        emp.userId = q.value(5).toInt();
        emp.badgeCode = q.value(6).toString();
        emp.updateDate = q.value(7).toDate();
        emp.isDeleted = q.value(8).toBool();
        emp.position.name = q.value(9).toString();
        result.append(emp);
    }
    m_lastError = QSqlError();
    return result;
}

Employee EmployeeManager::getEmployee(int id, bool includeDeleted)
{
    Employee emp;
    if (!m_db || !m_db->isConnected() || id <= 0)
        return emp;
    QSqlQuery q(m_db->getDatabase());
    QString sql = QStringLiteral(
        "SELECT e.id, e.lastname, e.firstname, e.middlename, e.positionid, e.userid, e.badgecode, e.updatedate, e.isdeleted, "
        "p.name AS positionname FROM employees e LEFT JOIN positions p ON p.id = e.positionid WHERE e.id = :id");
    if (!includeDeleted)
        sql += QStringLiteral(" AND (e.isdeleted IS NULL OR e.isdeleted = false)");
    q.prepare(sql);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError();
        return emp;
    }
    if (!q.next())
        return emp;
    emp.id = q.value(0).toInt();
    emp.lastName = q.value(1).toString();
    emp.firstName = q.value(2).toString();
    emp.middleName = q.value(3).toString();
    emp.position.id = q.value(4).toInt();
    emp.userId = q.value(5).toInt();
    emp.badgeCode = q.value(6).toString();
    emp.updateDate = q.value(7).toDate();
    emp.isDeleted = q.value(8).toBool();
    emp.position.name = q.value(9).toString();
    m_lastError = QSqlError();
    return emp;
}

bool EmployeeManager::add(Employee &emp)
{
    if (!m_db || !m_db->isConnected() || emp.position.id <= 0)
        return false;
    const QString last = escapeSql(emp.lastName.trimmed());
    const QString first = escapeSql(emp.firstName.trimmed());
    const QString middle = escapeSql(emp.middleName.trimmed());
    const QString userVal = emp.userId > 0 ? QString::number(emp.userId) : QStringLiteral("NULL");
    const QString badge = emp.badgeCode.trimmed().isEmpty()
        ? QStringLiteral("NULL") : QStringLiteral("'%1'").arg(escapeSql(emp.badgeCode.trimmed()));
    const QString sql = QStringLiteral(
        "INSERT INTO employees (lastname, firstname, middlename, positionid, userid, badgecode, updatedate, isdeleted) "
        "VALUES ('%1', '%2', '%3', %4, %5, %6, CURRENT_DATE, false) RETURNING id")
        .arg(last).arg(first).arg(middle).arg(emp.position.id).arg(userVal).arg(badge);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    if (q.next())
        emp.id = q.value(0).toInt();
    m_lastError = QSqlError();
    return true;
}

bool EmployeeManager::update(const Employee &emp)
{
    if (!m_db || !m_db->isConnected() || emp.id <= 0 || emp.position.id <= 0)
        return false;
    const QString last = escapeSql(emp.lastName.trimmed());
    const QString first = escapeSql(emp.firstName.trimmed());
    const QString middle = escapeSql(emp.middleName.trimmed());
    const QString userVal = emp.userId > 0 ? QString::number(emp.userId) : QStringLiteral("NULL");
    const QString badge = emp.badgeCode.trimmed().isEmpty()
        ? QStringLiteral("NULL") : QStringLiteral("'%1'").arg(escapeSql(emp.badgeCode.trimmed()));
    const QString sql = QStringLiteral(
        "UPDATE employees SET lastname = '%1', firstname = '%2', middlename = '%3', "
        "positionid = %4, userid = %5, badgecode = %6, updatedate = CURRENT_DATE WHERE id = %7")
        .arg(last).arg(first).arg(middle).arg(emp.position.id).arg(userVal).arg(badge).arg(emp.id);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}

bool EmployeeManager::setDeleted(int id, bool deleted)
{
    if (!m_db || !m_db->isConnected() || id <= 0)
        return false;
    const QString delVal = deleted ? QStringLiteral("true") : QStringLiteral("false");
    const QString sql = QStringLiteral("UPDATE employees SET isdeleted = %1, updatedate = CURRENT_DATE WHERE id = %2").arg(delVal).arg(id);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}
