#ifndef PROMOTIONMANAGER_H
#define PROMOTIONMANAGER_H

#include <QObject>
#include <QList>
#include <QSqlError>
#include "structures.h"

class DatabaseConnection;

class PromotionManager : public QObject
{
    Q_OBJECT
public:
    explicit PromotionManager(QObject *parent = nullptr);
    void setDatabaseConnection(DatabaseConnection *conn);

    QList<Promotion> list(bool includeDeleted = false);
    bool add(Promotion &promo);
    bool update(const Promotion &promo);
    bool setDeleted(int id, bool deleted = true);

    QSqlError lastError() const { return m_lastError; }

private:
    DatabaseConnection *m_db = nullptr;
    mutable QSqlError m_lastError;
};

#endif // PROMOTIONMANAGER_H
