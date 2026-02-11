#ifndef ABSTRACTREPORTEXPORTER_H
#define ABSTRACTREPORTEXPORTER_H

#include "reportdata.h"
#include <QString>

/**
 * Абстрактный экспортёр отчётов.
 * Реализации: CSV (CsvReportExporter), в будущем — PDF, Excel и т.д.
 */
class AbstractReportExporter
{
public:
    virtual ~AbstractReportExporter() = default;

    /**
     * Экспортировать отчёт в файл.
     * @param data данные отчёта (заголовок, колонки, строки)
     * @param filePath путь к файлу
     * @return true при успехе
     */
    virtual bool exportReport(const ReportData &data, const QString &filePath) = 0;

    /** Краткое описание формата (например, "CSV (UTF-8)") */
    virtual QString formatDescription() const = 0;

    /** Расширение файла по умолчанию (например, "csv") */
    virtual QString defaultFileExtension() const = 0;
};

#endif // ABSTRACTREPORTEXPORTER_H
