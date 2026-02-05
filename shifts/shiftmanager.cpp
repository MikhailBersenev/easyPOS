#include "shiftmanager.h"
#include "logging/logmanager.h"
#include <QSqlQuery>
#include <QSqlError>

ShiftManager::ShiftManager(QObject *parent) : QObject(parent), m_db(nullptr) {}

void ShiftManager::setDatabaseConnection(DatabaseConnection *conn)
{
    m_db = conn;
}

bool ShiftManager::startShift(qint64 employeeId, const QString &comment)
{
    if (!m_db || !m_db->isConnected() || employeeId <= 0) {
        LOG_WARNING() << "ShiftManager::startShift: нет БД или employeeId" << employeeId;
        return false;
    }
    WorkShift current = getCurrentShift(employeeId);
    if (current.id != 0) {
        LOG_INFO() << "ShiftManager::startShift: у сотрудника" << employeeId << "уже есть открытая смена id=" << current.id;
        return false; // уже есть открытая смена
    }
    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO workshifts (employeeid, starteddate, startedtime, comment) VALUES (:eid, CURRENT_DATE, CURRENT_TIME, :comment) RETURNING id"));
    q.bindValue(QStringLiteral(":eid"), employeeId);
    q.bindValue(QStringLiteral(":comment"), comment.trimmed());
    if (!q.exec() || !q.next()) {
        LOG_WARNING() << "ShiftManager::startShift: ошибка INSERT для employeeId" << employeeId << q.lastError().text();
        return false;
    }
    qint64 shiftId = q.value(0).toLongLong();
    LOG_INFO() << "ShiftManager::startShift: смена начата employeeId=" << employeeId << "shiftId=" << shiftId;
    emit shiftStarted(employeeId, shiftId);
    return true;
}

bool ShiftManager::endCurrentShift(qint64 employeeId, const QString &comment)
{
    if (!m_db || !m_db->isConnected() || employeeId <= 0) {
        LOG_WARNING() << "ShiftManager::endCurrentShift: нет БД или employeeId" << employeeId;
        return false;
    }
    WorkShift current = getCurrentShift(employeeId);
    if (current.id == 0) {
        LOG_INFO() << "ShiftManager::endCurrentShift: у сотрудника" << employeeId << "нет открытой смены";
        return false;
    }
    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "UPDATE workshifts SET endeddate = CURRENT_DATE, endedtime = CURRENT_TIME, comment = COALESCE(NULLIF(trim(:comment), ''), comment) WHERE id = :id"));
    q.bindValue(QStringLiteral(":comment"), comment);
    q.bindValue(QStringLiteral(":id"), current.id);
    if (!q.exec()) {
        LOG_WARNING() << "ShiftManager::endCurrentShift: ошибка UPDATE shiftId=" << current.id << q.lastError().text();
        return false;
    }
    LOG_INFO() << "ShiftManager::endCurrentShift: смена завершена employeeId=" << employeeId << "shiftId=" << current.id;
    emit shiftEnded(employeeId, current.id);
    return true;
}

WorkShift ShiftManager::getCurrentShift(qint64 employeeId) const
{
    WorkShift s;
    if (!m_db || !m_db->isConnected() || employeeId <= 0) return s;
    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT ws.id, ws.employeeid, ws.starteddate, ws.startedtime, ws.endeddate, ws.endedtime, ws.comment, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS empname "
        "FROM workshifts ws LEFT JOIN employees e ON e.id = ws.employeeid "
        "WHERE ws.employeeid = :eid AND ws.endeddate IS NULL ORDER BY ws.starteddate DESC, ws.startedtime DESC LIMIT 1"));
    q.bindValue(QStringLiteral(":eid"), employeeId);
    if (!q.exec() || !q.next()) return s;
    s.id = q.value(0).toLongLong();
    s.employeeId = q.value(1).toLongLong();
    s.startedDate = q.value(2).toDate();
    s.startedTime = q.value(3).toTime();
    s.endedDate = q.value(4).toDate();
    s.endedTime = q.value(5).toTime();
    s.comment = q.value(6).toString();
    s.employeeName = q.value(7).toString().trimmed();
    LOG_DEBUG() << "ShiftManager::getCurrentShift: employeeId=" << employeeId << "найден shiftId=" << s.id;
    return s;
}

QList<WorkShift> ShiftManager::getShifts(qint64 employeeId, const QDate &dateFrom, const QDate &dateTo) const
{
    QList<WorkShift> list;
    if (!m_db || !m_db->isConnected()) return list;
    QSqlQuery q(m_db->getDatabase());
    QString sql = QStringLiteral(
        "SELECT ws.id, ws.employeeid, ws.starteddate, ws.startedtime, ws.endeddate, ws.endedtime, ws.comment, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS empname "
        "FROM workshifts ws LEFT JOIN employees e ON e.id = ws.employeeid WHERE 1=1 ");
    if (employeeId > 0) {
        sql += QStringLiteral("AND ws.employeeid = :eid ");
    }
    sql += QStringLiteral("AND ws.starteddate BETWEEN :dfrom AND :dto ORDER BY ws.starteddate DESC, ws.startedtime DESC");
    q.prepare(sql);
    if (employeeId > 0) q.bindValue(QStringLiteral(":eid"), employeeId);
    q.bindValue(QStringLiteral(":dfrom"), dateFrom);
    q.bindValue(QStringLiteral(":dto"), dateTo);
    if (!q.exec()) return list;
    while (q.next()) {
        WorkShift s;
        s.id = q.value(0).toLongLong();
        s.employeeId = q.value(1).toLongLong();
        s.startedDate = q.value(2).toDate();
        s.startedTime = q.value(3).toTime();
        s.endedDate = q.value(4).toDate();
        s.endedTime = q.value(5).toTime();
        s.comment = q.value(6).toString();
        s.employeeName = q.value(7).toString().trimmed();
        list.append(s);
    }
    LOG_DEBUG() << "ShiftManager::getShifts: employeeId=" << employeeId << "period" << dateFrom << "-" << dateTo << "найдено смен:" << list.size();
    return list;
}
