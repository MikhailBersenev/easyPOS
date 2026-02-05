#include "shiftmanager.h"
#include "logging/logmanager.h"
#include <QSqlQuery>
#include <QSqlError>

ShiftManager::ShiftManager(QObject *parent) : QObject(parent), m_db(nullptr) {}

void ShiftManager::setDatabaseConnection(DatabaseConnection *conn)
{
    qDebug() << "ShiftManager::setDatabaseConnection" << (conn ? "set" : "null");
    m_db = conn;
}

bool ShiftManager::startShift(qint64 employeeId, const QString &comment)
{
    qDebug() << "ShiftManager::startShift" << "employeeId=" << employeeId;
    if (!m_db || !m_db->isConnected() || employeeId <= 0) {
        LOG_WARNING() << "ShiftManager::startShift: нет БД или employeeId" << employeeId;
        return false;
    }
    WorkShift current = getCurrentShift(employeeId);
    if (current.id != 0) {
        LOG_INFO() << "ShiftManager::startShift: у сотрудника" << employeeId << "уже есть открытая смена id=" << current.id;
        qDebug() << "ShiftManager::startShift: branch already_open";
        return false; // уже есть открытая смена
    }
    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "INSERT INTO workshifts (employeeid, starteddate, startedtime, comment) VALUES (:eid, CURRENT_DATE, CURRENT_TIME, :comment) RETURNING id"));
    q.bindValue(QStringLiteral(":eid"), employeeId);
    q.bindValue(QStringLiteral(":comment"), comment.trimmed());
    if (!q.exec() || !q.next()) {
        LOG_WARNING() << "ShiftManager::startShift: ошибка INSERT для employeeId" << employeeId << q.lastError().text();
        qDebug() << "ShiftManager::startShift: branch insert_failed";
        return false;
    }
    qint64 shiftId = q.value(0).toLongLong();
    qDebug() << "ShiftManager::startShift: branch success shiftId=" << shiftId;
    LOG_INFO() << "ShiftManager::startShift: смена начата employeeId=" << employeeId << "shiftId=" << shiftId;
    emit shiftStarted(employeeId, shiftId);
    return true;
}

bool ShiftManager::endCurrentShift(qint64 employeeId, const QString &comment)
{
    qDebug() << "ShiftManager::endCurrentShift" << "employeeId=" << employeeId;
    if (!m_db || !m_db->isConnected() || employeeId <= 0) {
        LOG_WARNING() << "ShiftManager::endCurrentShift: нет БД или employeeId" << employeeId;
        return false;
    }
    WorkShift current = getCurrentShift(employeeId);
    if (current.id == 0) {
        LOG_INFO() << "ShiftManager::endCurrentShift: у сотрудника" << employeeId << "нет открытой смены";
        qDebug() << "ShiftManager::endCurrentShift: branch no_open_shift";
        return false;
    }
    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "UPDATE workshifts SET endeddate = CURRENT_DATE, endedtime = CURRENT_TIME, comment = COALESCE(NULLIF(trim(:comment), ''), comment) WHERE id = :id"));
    q.bindValue(QStringLiteral(":comment"), comment);
    q.bindValue(QStringLiteral(":id"), current.id);
    if (!q.exec()) {
        LOG_WARNING() << "ShiftManager::endCurrentShift: ошибка UPDATE shiftId=" << current.id << q.lastError().text();
        qDebug() << "ShiftManager::endCurrentShift: branch update_failed";
        return false;
    }
    qDebug() << "ShiftManager::endCurrentShift: branch success";
    LOG_INFO() << "ShiftManager::endCurrentShift: смена завершена employeeId=" << employeeId << "shiftId=" << current.id;
    emit shiftEnded(employeeId, current.id);
    return true;
}

WorkShift ShiftManager::getCurrentShift(qint64 employeeId) const
{
    qDebug() << "ShiftManager::getCurrentShift" << "employeeId=" << employeeId;
    WorkShift s;
    if (!m_db || !m_db->isConnected() || employeeId <= 0) {
        qDebug() << "ShiftManager::getCurrentShift: branch skip (no db or bad id)";
        return s;
    }
    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT ws.id, ws.employeeid, ws.starteddate, ws.startedtime, ws.endeddate, ws.endedtime, ws.comment, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS empname "
        "FROM workshifts ws LEFT JOIN employees e ON e.id = ws.employeeid "
        "WHERE ws.employeeid = :eid AND ws.endeddate IS NULL ORDER BY ws.starteddate DESC, ws.startedtime DESC LIMIT 1"));
    q.bindValue(QStringLiteral(":eid"), employeeId);
    if (!q.exec() || !q.next()) {
        qDebug() << "ShiftManager::getCurrentShift: branch no_row";
        return s;
    }
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
    qDebug() << "ShiftManager::getShifts" << "employeeId=" << employeeId << "from=" << dateFrom << "to=" << dateTo;
    QList<WorkShift> list;
    if (!m_db || !m_db->isConnected()) {
        qDebug() << "ShiftManager::getShifts: branch no_db";
        return list;
    }
    QSqlQuery q(m_db->getDatabase());
    QString sql = QStringLiteral(
        "SELECT ws.id, ws.employeeid, ws.starteddate, ws.startedtime, ws.endeddate, ws.endedtime, ws.comment, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS empname "
        "FROM workshifts ws LEFT JOIN employees e ON e.id = ws.employeeid WHERE 1=1 ");
    if (employeeId > 0) {
        qDebug() << "ShiftManager::getShifts: branch filter_by_employee";
        sql += QStringLiteral("AND ws.employeeid = :eid ");
    }
    sql += QStringLiteral("AND ws.starteddate BETWEEN :dfrom AND :dto ORDER BY ws.starteddate DESC, ws.startedtime DESC");
    q.prepare(sql);
    if (employeeId > 0) q.bindValue(QStringLiteral(":eid"), employeeId);
    q.bindValue(QStringLiteral(":dfrom"), dateFrom);
    q.bindValue(QStringLiteral(":dto"), dateTo);
    if (!q.exec()) {
        qDebug() << "ShiftManager::getShifts: branch query_failed";
        return list;
    }
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
