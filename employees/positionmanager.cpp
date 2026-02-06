#include "positionmanager.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

PositionManager::PositionManager(QObject *parent) : QObject(parent), m_db(nullptr) {}

static QString escapeSql(const QString &s)
{
    return QString(s).replace(QLatin1Char('\''), QLatin1String("''"));
}

void PositionManager::setDatabaseConnection(DatabaseConnection *conn)
{
    m_db = conn;
}

QList<Position> PositionManager::list(bool includeDeleted)
{
    QList<Position> result;
    if (!m_db || !m_db->isConnected())
        return result;

    QSqlQuery q(m_db->getDatabase());
    QString sql = QStringLiteral("SELECT id, name, isactive, updatedate, isdeleted FROM positions");
    if (!includeDeleted)
        sql += QStringLiteral(" WHERE (isdeleted IS NULL OR isdeleted = false)");
    sql += QStringLiteral(" ORDER BY name");
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return result;
    }
    while (q.next()) {
        Position p;
        p.id = q.value(0).toInt();
        p.name = q.value(1).toString();
        p.isActive = q.value(2).toBool();
        p.updateDate = q.value(3).toDate();
        p.isDeleted = q.value(4).toBool();
        result.append(p);
    }
    m_lastError = QSqlError();
    return result;
}

bool PositionManager::add(Position &pos)
{
    if (!m_db || !m_db->isConnected())
        return false;
    const QString name = escapeSql(pos.name.trimmed());
    const QString isActiveVal = pos.isActive ? QStringLiteral("true") : QStringLiteral("false");
    const QString sql = QStringLiteral(
        "INSERT INTO positions (name, isactive, updatedate, isdeleted) "
        "VALUES ('%1', %2, CURRENT_DATE, false) RETURNING id").arg(name).arg(isActiveVal);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    if (q.next())
        pos.id = q.value(0).toInt();
    m_lastError = QSqlError();
    return true;
}

bool PositionManager::update(const Position &pos)
{
    if (!m_db || !m_db->isConnected() || pos.id <= 0)
        return false;
    const QString name = escapeSql(pos.name.trimmed());
    const QString isActiveVal = pos.isActive ? QStringLiteral("true") : QStringLiteral("false");
    const QString sql = QStringLiteral(
        "UPDATE positions SET name = '%1', isactive = %2, updatedate = CURRENT_DATE WHERE id = %3")
        .arg(name).arg(isActiveVal).arg(pos.id);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}

bool PositionManager::setDeleted(int id, bool deleted)
{
    if (!m_db || !m_db->isConnected() || id <= 0)
        return false;
    const QString delVal = deleted ? QStringLiteral("true") : QStringLiteral("false");
    const QString sql = QStringLiteral("UPDATE positions SET isdeleted = %1, updatedate = CURRENT_DATE WHERE id = %2").arg(delVal).arg(id);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}
