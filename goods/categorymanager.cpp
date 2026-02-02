#include "categorymanager.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDate>

CategoryManager::CategoryManager(QObject *parent)
    : QObject(parent)
    , m_db(nullptr)
{
}

CategoryManager::~CategoryManager()
{
}

void CategoryManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    m_db = dbConnection;
}

QList<GoodCategory> CategoryManager::list(bool includeDeleted)
{
    QList<GoodCategory> result;
    if (!m_db || !m_db->isConnected())
        return result;

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    QString sql = QStringLiteral(
        "SELECT id, name, description, updatedate, isdeleted FROM goodcats");
    if (!includeDeleted)
        sql += QStringLiteral(" WHERE (isdeleted IS NULL OR isdeleted = false)");
    sql += QStringLiteral(" ORDER BY name");
    if (!q.exec(sql)) {
        m_lastError = q.lastError();
        return result;
    }

    while (q.next()) {
        GoodCategory cat;
        cat.id = q.value(0).toInt();
        cat.name = q.value(1).toString();
        cat.description = q.value(2).toString();
        cat.updateDate = q.value(3).toDate();
        cat.isDeleted = q.value(4).toBool();
        cat.isActive = !cat.isDeleted;
        result.append(cat);
    }
    return result;
}

bool CategoryManager::add(GoodCategory &cat)
{
    if (!m_db || !m_db->isConnected())
        return false;

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO goodcats (name, description, updatedate, isdeleted) "
        "VALUES (:name, :description, :updatedate, false) RETURNING id"));
    q.bindValue(QStringLiteral(":name"), cat.name);
    q.bindValue(QStringLiteral(":description"), cat.description);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    if (!q.exec()) {
        m_lastError = q.lastError();
        return false;
    }
    if (q.next())
        cat.id = q.value(0).toInt();
    return true;
}

bool CategoryManager::update(const GoodCategory &cat)
{
    if (!m_db || !m_db->isConnected() || cat.id <= 0)
        return false;

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE goodcats SET name = :name, description = :description, "
        "updatedate = :updatedate WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"), cat.name);
    q.bindValue(QStringLiteral(":description"), cat.description);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    q.bindValue(QStringLiteral(":id"), cat.id);
    if (!q.exec()) {
        m_lastError = q.lastError();
        return false;
    }
    return true;
}

bool CategoryManager::setDeleted(int id, bool deleted)
{
    if (!m_db || !m_db->isConnected() || id <= 0)
        return false;

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE goodcats SET isdeleted = :deleted WHERE id = :id"));
    q.bindValue(QStringLiteral(":deleted"), deleted);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError();
        return false;
    }
    return true;
}
