#ifndef CSVREPORTEXPORTER_H
#define CSVREPORTEXPORTER_H

#include "abstractreportexporter.h"

/**
 * Экспорт отчётов в формат CSV (разделитель — точка с запятой, кодировка UTF-8).
 */
class CsvReportExporter : public AbstractReportExporter
{
public:
    CsvReportExporter() = default;

    bool exportReport(const ReportData &data, const QString &filePath) override;
    QString formatDescription() const override;
    QString defaultFileExtension() const override;

private:
    static QString escapeCsvField(const QString &value);
};

#endif // CSVREPORTEXPORTER_H
