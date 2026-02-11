#ifndef ABSTRACTREPORT_H
#define ABSTRACTREPORT_H

#include "reportdata.h"
#include <QVariantMap>

/**
 * Абстрактный генератор отчёта. В будущем можно добавлять новые типы отчётов,
 * реализующие этот интерфейс (например, отчёт по категориям, по сотрудникам).
 */
class AbstractReport
{
public:
    virtual ~AbstractReport() = default;

    /**
     * Сформировать данные отчёта.
     * @param params параметры (например, dateFrom, dateTo, shiftId)
     * @return данные для экспорта
     */
    virtual ReportData generate(const QVariantMap &params) = 0;

    /** Идентификатор/название типа отчёта */
    virtual QString reportId() const = 0;
};

#endif // ABSTRACTREPORT_H
