#include "promotionmanager.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>

PromotionManager::PromotionManager(QObject *parent) : QObject(parent), m_db(nullptr) {}

static QString escapeSql(const QString &s)
{
    return QString(s).replace(QLatin1Char('\''), QLatin1String("''"));
}

void PromotionManager::setDatabaseConnection(DatabaseConnection *conn)
{
    m_db = conn;
}

QList<Promotion> PromotionManager::list(bool includeDeleted)
{
    QList<Promotion> result;
    if (!m_db || !m_db->isConnected())
        return result;

    QSqlQuery q(m_db->getDatabase());
    QString sql = QStringLiteral("SELECT id, name, description, percent, sum, isdeleted FROM discounts");
    if (!includeDeleted)
        sql += QStringLiteral(" WHERE (isdeleted IS NULL OR isdeleted = false)");
    sql += QStringLiteral(" ORDER BY name");
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return result;
    }
    while (q.next()) {
        Promotion p;
        p.id = q.value(0).toInt();
        p.name = q.value(1).toString();
        p.description = q.value(2).toString();
        p.percent = q.value(3).toDouble();
        p.sum = q.value(4).toDouble();
        p.isDeleted = q.value(5).toBool();
        result.append(p);
    }
    m_lastError = QSqlError();
    return result;
}

bool PromotionManager::add(Promotion &promo)
{
    if (!m_db || !m_db->isConnected())
        return false;
    const QString name = escapeSql(promo.name.trimmed());
    const QString desc = escapeSql(promo.description.trimmed());
    const QString sql = QStringLiteral(
        "INSERT INTO discounts (name, description, percent, \"sum\", isdeleted) "
        "VALUES ('%1', '%2', %3, %4, false) RETURNING id")
        .arg(name).arg(desc).arg(promo.percent).arg(promo.sum);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    if (q.next())
        promo.id = q.value(0).toInt();
    m_lastError = QSqlError();
    return true;
}

bool PromotionManager::update(const Promotion &promo)
{
    if (!m_db || !m_db->isConnected() || promo.id <= 0)
        return false;
    const QString name = escapeSql(promo.name.trimmed());
    const QString desc = escapeSql(promo.description.trimmed());
    const QString sql = QStringLiteral(
        "UPDATE discounts SET name = '%1', description = '%2', percent = %3, \"sum\" = %4 WHERE id = %5")
        .arg(name).arg(desc).arg(promo.percent).arg(promo.sum).arg(promo.id);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}

bool PromotionManager::setDeleted(int id, bool deleted)
{
    if (!m_db || !m_db->isConnected() || id <= 0)
        return false;
    const QString delVal = deleted ? QStringLiteral("true") : QStringLiteral("false");
    const QString sql = QStringLiteral("UPDATE discounts SET isdeleted = %1 WHERE id = %2").arg(delVal).arg(id);
    QSqlQuery q(m_db->getDatabase());
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return false;
    }
    m_lastError = QSqlError();
    return true;
}
