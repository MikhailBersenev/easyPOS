#ifndef POSITIONMANAGER_H
#define POSITIONMANAGER_H

#include <QObject>
#include <QList>
#include <QSqlError>
#include "structures.h"

class DatabaseConnection;

class PositionManager : public QObject
{
    Q_OBJECT
public:
    explicit PositionManager(QObject *parent = nullptr);
    void setDatabaseConnection(DatabaseConnection *conn);

    QList<Position> list(bool includeDeleted = false);
    bool add(Position &pos);
    bool update(const Position &pos);
    bool setDeleted(int id, bool deleted = true);

    QSqlError lastError() const { return m_lastError; }

private:
    DatabaseConnection *m_db = nullptr;
    mutable QSqlError m_lastError;
};

#endif // POSITIONMANAGER_H
