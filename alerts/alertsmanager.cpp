#include "alertsmanager.h"
#include "logging/logmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

AlertsManager::AlertsManager(QObject *parent) : QObject(parent), m_db(nullptr) {}

void AlertsManager::setDatabaseConnection(DatabaseConnection *conn)
{
    m_db = conn;
}

// Экранирование строки для PostgreSQL (одиночная кавычка -> две)
static QString escapeSqlString(const QString &s)
{
    return QString(s).replace(QLatin1Char('\''), QLatin1String("''"));
}

static QString sqlLiteral(const QString &s)
{
    return QStringLiteral("'") + escapeSqlString(s) + QLatin1Char('\'');
}

bool AlertsManager::log(const QString &category, const QString &signature, const QString &fullLog,
                        qint64 userId, qint64 employeeId)
{
    if (!m_db || !m_db->isConnected()) {
        m_lastError = QSqlError(QString(), QStringLiteral("AlertsManager: нет подключения к БД"), QSqlError::ConnectionError);
        return false;
    }
    const QString cat = category.trimmed();
    const QString sig = signature.trimmed();
    const QString uidVal = userId > 0 ? QString::number(userId) : QStringLiteral("NULL");
    const QString eidVal = employeeId > 0 ? QString::number(employeeId) : QStringLiteral("NULL");
    const QString sql = QStringLiteral(
        "INSERT INTO alerts (category, signature, fulllog, userid, employeeid) "
        "VALUES (%1, %2, %3, %4, %5)")
        .arg(sqlLiteral(cat), sqlLiteral(sig), sqlLiteral(fullLog), uidVal, eidVal);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        LOG_WARNING() << "AlertsManager::log:" << m_lastError.text();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}

bool AlertsManager::logAt(const QString &category, const QString &signature, const QString &fullLog,
                          const QDate &eventDate, const QTime &eventTime,
                          qint64 userId, qint64 employeeId)
{
    if (!m_db || !m_db->isConnected()) {
        m_lastError = QSqlError(QString(), QStringLiteral("AlertsManager: нет подключения к БД"), QSqlError::ConnectionError);
        return false;
    }
    const QString cat = category.trimmed();
    const QString sig = signature.trimmed();
    const QString uidVal = userId > 0 ? QString::number(userId) : QStringLiteral("NULL");
    const QString eidVal = employeeId > 0 ? QString::number(employeeId) : QStringLiteral("NULL");
    const QString dateStr = sqlLiteral(eventDate.toString(Qt::ISODate));
    const QString timeStr = sqlLiteral(eventTime.toString(QStringLiteral("HH:mm:ss")));
    const QString sql = QStringLiteral(
        "INSERT INTO alerts (category, signature, fulllog, eventdate, eventtime, userid, employeeid) "
        "VALUES (%1, %2, %3, %4::date, %5::time, %6, %7)")
        .arg(sqlLiteral(cat), sqlLiteral(sig), sqlLiteral(fullLog), dateStr, timeStr, uidVal, eidVal);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        LOG_WARNING() << "AlertsManager::logAt:" << m_lastError.text();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}

static Alert alertFromQuery(const QSqlQuery &q)
{
    Alert a;
    a.id = q.value(0).toLongLong();
    a.category = q.value(1).toString();
    a.signature = q.value(2).toString();
    a.fullLog = q.value(3).toString();
    a.eventDate = q.value(4).toDate();
    a.eventTime = q.value(5).toTime();
    a.userId = q.value(6).toLongLong();
    a.employeeId = q.value(7).toLongLong();
    a.createdAt = q.value(8).toDateTime();
    if (q.value(9).isValid())
        a.userName = q.value(9).toString();
    if (q.value(10).isValid())
        a.employeeName = q.value(10).toString().trimmed();
    return a;
}

QList<Alert> AlertsManager::getAlerts(const QDate &dateFrom, const QDate &dateTo,
                                      const QString &category,
                                      qint64 userId, qint64 employeeId,
                                      int limit) const
{
    QList<Alert> list;
    if (!m_db || !m_db->isConnected())
        return list;

    QString sql = QStringLiteral(
        "SELECT a.id, a.category, a.signature, a.fulllog, a.eventdate, a.eventtime, "
        "a.userid, a.employeeid, a.createdat, "
        "u.name AS username, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS employeename "
        "FROM alerts a "
        "LEFT JOIN users u ON u.id = a.userid "
        "LEFT JOIN employees e ON e.id = a.employeeid "
        "WHERE a.eventdate >= :datefrom AND a.eventdate <= :dateto ");
    if (!category.isEmpty())
        sql += QStringLiteral("AND a.category = :category ");
    if (userId > 0)
        sql += QStringLiteral("AND a.userid = :userid ");
    if (employeeId > 0)
        sql += QStringLiteral("AND a.employeeid = :employeeid ");
    sql += QStringLiteral("ORDER BY a.eventdate DESC, a.eventtime DESC, a.id DESC ");
    if (limit > 0)
        sql += QStringLiteral("LIMIT :limit");

    QSqlQuery q(m_db->getDatabase());
    q.prepare(sql);
    q.bindValue(QStringLiteral(":datefrom"), dateFrom);
    q.bindValue(QStringLiteral(":dateto"), dateTo);
    if (!category.isEmpty())
        q.bindValue(QStringLiteral(":category"), category);
    if (userId > 0)
        q.bindValue(QStringLiteral(":userid"), userId);
    if (employeeId > 0)
        q.bindValue(QStringLiteral(":employeeid"), employeeId);
    if (limit > 0)
        q.bindValue(QStringLiteral(":limit"), limit);

    if (!q.exec()) {
        m_lastError = q.lastError();
        return list;
    }
    while (q.next())
        list.append(alertFromQuery(q));
    m_lastError = QSqlError();
    return list;
}

Alert AlertsManager::getAlert(qint64 alertId) const
{
    Alert a;
    if (!m_db || !m_db->isConnected() || alertId <= 0)
        return a;

    QSqlQuery q(m_db->getDatabase());
    q.prepare(QStringLiteral(
        "SELECT a.id, a.category, a.signature, a.fulllog, a.eventdate, a.eventtime, "
        "a.userid, a.employeeid, a.createdat, "
        "u.name AS username, "
        "TRIM(COALESCE(e.lastname,'') || ' ' || COALESCE(e.firstname,'')) AS employeename "
        "FROM alerts a "
        "LEFT JOIN users u ON u.id = a.userid "
        "LEFT JOIN employees e ON e.id = a.employeeid "
        "WHERE a.id = :id"));
    q.bindValue(QStringLiteral(":id"), alertId);
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError();
        return a;
    }
    m_lastError = QSqlError();
    return alertFromQuery(q);
}
