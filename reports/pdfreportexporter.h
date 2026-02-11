#ifndef PDFREPORTEXPORTER_H
#define PDFREPORTEXPORTER_H

#include "abstractreportexporter.h"

/**
 * Экспорт отчётов в формат PDF.
 * Структура: логотип, реквизиты предприятия, название отчёта, сводка, таблица, подпись и печать.
 */
class PdfReportExporter : public AbstractReportExporter
{
public:
    PdfReportExporter() = default;

    bool exportReport(const ReportData &data, const QString &filePath) override;
    QString formatDescription() const override;
    QString defaultFileExtension() const override;

    void setLogoPath(const QString &path) { m_logoPath = path; }
    void setCompanyName(const QString &name) { m_companyName = name; }
    void setAddress(const QString &address) { m_address = address; }
    void setLegalInfo(const QString &info) { m_legalInfo = info; }

private:
    QString m_logoPath;
    QString m_companyName;
    QString m_address;
    QString m_legalInfo;
};

#endif // PDFREPORTEXPORTER_H
