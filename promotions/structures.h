#ifndef PROMOTIONS_STRUCTURES_H
#define PROMOTIONS_STRUCTURES_H

#include <QString>

struct Promotion {
    int id;
    QString name;
    QString description;
    double percent;   // скидка в %
    double sum;       // скидка суммой (руб)
    bool isDeleted;

    Promotion() : id(0), percent(0), sum(0), isDeleted(false) {}
};

#endif // PROMOTIONS_STRUCTURES_H
