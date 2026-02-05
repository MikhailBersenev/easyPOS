#ifndef SHIFTS_STRUCTURES_H
#define SHIFTS_STRUCTURES_H

#include <QString>
#include <QDate>
#include <QTime>

struct WorkShift {
    qint64 id = 0;
    qint64 employeeId = 0;
    QString employeeName;
    QDate startedDate;
    QTime startedTime;
    QDate endedDate;
    QTime endedTime;
    QString comment;
    bool isActive() const { return !endedDate.isValid(); }
};

#endif // SHIFTS_STRUCTURES_H
