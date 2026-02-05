#include "categorymanager.h"
#include "../db/databaseconnection.h"
#include "../logging/logmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDate>

CategoryManager::CategoryManager(QObject *parent)
    : QObject(parent)
    , m_db(nullptr)
{
    qDebug() << "CategoryManager::CategoryManager";
}

CategoryManager::~CategoryManager()
{
    qDebug() << "CategoryManager::~CategoryManager";
}

void CategoryManager::setDatabaseConnection(DatabaseConnection *dbConnection)
{
    qDebug() << "CategoryManager::setDatabaseConnection" << (dbConnection ? "set" : "null");
    m_db = dbConnection;
}

QList<GoodCategory> CategoryManager::list(bool includeDeleted)
{
    qDebug() << "CategoryManager::list" << "includeDeleted=" << includeDeleted;
    QList<GoodCategory> result;
    if (!m_db || !m_db->isConnected()) {
        qDebug() << "CategoryManager::list: branch no_db";
        return result;
    }

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    QString sql = QStringLiteral(
        "SELECT id, name, description, updatedate, isdeleted FROM goodcats");
    if (!includeDeleted) {
        qDebug() << "CategoryManager::list: branch exclude_deleted";
        sql += QStringLiteral(" WHERE (isdeleted IS NULL OR isdeleted = false)");
    }
    sql += QStringLiteral(" ORDER BY name");
    if (!q.exec(sql)) {
        qDebug() << "CategoryManager::list: branch query_failed" << q.lastError().text();
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
    qDebug() << "CategoryManager::add" << "name=" << cat.name;
    if (!m_db || !m_db->isConnected()) {
        qDebug() << "CategoryManager::add: branch no_db";
        return false;
    }

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO goodcats (name, description, updatedate, isdeleted) "
        "VALUES (:name, :description, :updatedate, false) RETURNING id"));
    q.bindValue(QStringLiteral(":name"), cat.name);
    q.bindValue(QStringLiteral(":description"), cat.description);
    q.bindValue(QStringLiteral(":updatedate"), QDate::currentDate());
    if (!q.exec()) {
        qDebug() << "CategoryManager::add: branch exec_failed" << q.lastError().text();
        m_lastError = q.lastError();
        return false;
    }
    if (q.next()) {
        cat.id = q.value(0).toInt();
        qDebug() << "CategoryManager::add: branch success id=" << cat.id;
    } else {
        qDebug() << "CategoryManager::add: branch no_returning_id";
    }
    return true;
}

bool CategoryManager::update(const GoodCategory &cat)
{
    qDebug() << "CategoryManager::update" << "id=" << cat.id;
    if (!m_db || !m_db->isConnected() || cat.id <= 0) {
        qDebug() << "CategoryManager::update: branch no_db_or_bad_id";
        return false;
    }

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
        qDebug() << "CategoryManager::update: branch exec_failed" << q.lastError().text();
        m_lastError = q.lastError();
        return false;
    }
    qDebug() << "CategoryManager::update: branch success";
    return true;
}

bool CategoryManager::setDeleted(int id, bool deleted)
{
    qDebug() << "CategoryManager::setDeleted" << "id=" << id << "deleted=" << deleted;
    if (!m_db || !m_db->isConnected() || id <= 0) {
        qDebug() << "CategoryManager::setDeleted: branch no_db_or_bad_id";
        return false;
    }

    QSqlDatabase db = m_db->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE goodcats SET isdeleted = :deleted WHERE id = :id"));
    q.bindValue(QStringLiteral(":deleted"), deleted);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qDebug() << "CategoryManager::setDeleted: branch exec_failed" << q.lastError().text();
        m_lastError = q.lastError();
        return false;
    }
    qDebug() << "CategoryManager::setDeleted: branch success";
    return true;
}
