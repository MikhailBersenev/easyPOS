#ifndef GOODS_STRUCTURES_H
#define GOODS_STRUCTURES_H

#include <QString>
#include <QDate>

struct GoodCategory {
    int id;
    QString name;
    QString description;
    bool isActive;
    QDate updateDate;
    bool isDeleted;
    
    GoodCategory() : id(0), isActive(true), isDeleted(false) {}
};

struct Good {
    int id;
    QString name;
    GoodCategory category;
    bool isActive;
    QDate updateDate;
    bool isDeleted;
    
    Good() : id(0), isActive(true), isDeleted(false) {}
};

#endif // GOODS_STRUCTURES_H

