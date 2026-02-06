#ifndef EMPLOYEES_STRUCTURES_H
#define EMPLOYEES_STRUCTURES_H

#include <QString>
#include <QDate>

struct Position {
    int id;
    QString name;
    bool isActive;
    QDate updateDate;
    bool isDeleted;
    
    Position() : id(0), isActive(true), isDeleted(false) {}
};

struct Employee {
    int id;
    QString lastName;
    QString firstName;
    QString middleName;
    Position position;
    int userId;  // ID пользователя (вместо вложенного объекта User)
    QString badgeCode;  // штрихкод бейджа для входа
    QDate updateDate;
    bool isDeleted;
    
    Employee() : id(0), userId(0), isDeleted(false) {}
};

#endif // EMPLOYEES_STRUCTURES_H

