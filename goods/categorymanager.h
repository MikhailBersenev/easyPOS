#ifndef CATEGORYMANAGER_H
#define CATEGORYMANAGER_H

#include <QObject>
#include <QList>
#include <QSqlError>
#include "structures.h"

class DatabaseConnection;

class CategoryManager : public QObject
{
    Q_OBJECT
public:
    explicit CategoryManager(QObject *parent = nullptr);
    ~CategoryManager();

    void setDatabaseConnection(DatabaseConnection *dbConnection);

    QList<GoodCategory> list(bool includeDeleted = false);
    bool add(GoodCategory &cat);
    bool update(const GoodCategory &cat);
    bool setDeleted(int id, bool deleted = true);

    QSqlError lastError() const { return m_lastError; }

private:
    DatabaseConnection *m_db = nullptr;
    QSqlError m_lastError;
};

#endif // CATEGORYMANAGER_H
