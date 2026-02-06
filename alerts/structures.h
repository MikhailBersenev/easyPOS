#ifndef ALERTS_STRUCTURES_H
#define ALERTS_STRUCTURES_H

#include <QString>
#include <QDate>
#include <QTime>
#include <QDateTime>

struct Alert {
    qint64 id = 0;
    QString category;
    QString signature;
    QString fullLog;
    QDate eventDate;
    QTime eventTime;
    qint64 userId = 0;
    qint64 employeeId = 0;
    QDateTime createdAt;
    /** Имя пользователя (из users), при выборке с JOIN */
    QString userName;
    /** Имя сотрудника (ФИО из employees), при выборке с JOIN */
    QString employeeName;
};

#endif // ALERTS_STRUCTURES_H
