#ifndef SHIFTMANAGER_H
#define SHIFTMANAGER_H

#include <QObject>
#include <QList>
#include <QDate>
#include "../db/databaseconnection.h"
#include "structures.h"

class ShiftManager : public QObject
{
    Q_OBJECT
public:
    explicit ShiftManager(QObject *parent = nullptr);
    void setDatabaseConnection(DatabaseConnection *conn);

    /** Начать смену для сотрудника */
    bool startShift(qint64 employeeId, const QString &comment = QString());
    /** Завершить текущую открытую смену сотрудника */
    bool endCurrentShift(qint64 employeeId, const QString &comment = QString());
    /** Текущая открытая смена сотрудника (endedat IS NULL) */
    WorkShift getCurrentShift(qint64 employeeId) const;
    /** Смены за период. employeeId == 0 — все сотрудники */
    QList<WorkShift> getShifts(qint64 employeeId, const QDate &dateFrom, const QDate &dateTo) const;

signals:
    void shiftStarted(qint64 employeeId, qint64 shiftId);
    void shiftEnded(qint64 employeeId, qint64 shiftId);

private:
    DatabaseConnection *m_db = nullptr;
};

#endif // SHIFTMANAGER_H
