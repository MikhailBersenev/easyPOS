#ifndef WORDREPORTEXPORTER_H
#define WORDREPORTEXPORTER_H

#include "abstractreportexporter.h"

/**
 * Экспорт отчётов в формат RTF (Rich Text Format).
 * Файлы открываются в Microsoft Word и других редакторах.
 * Поддерживает логотип предприятия, название, адрес и аналитику.
 */
class WordReportExporter : public AbstractReportExporter
{
public:
    WordReportExporter() = default;

    bool exportReport(const ReportData &data, const QString &filePath) override;
    QString formatDescription() const override;
    QString defaultFileExtension() const override;

    void setLogoPath(const QString &path) { m_logoPath = path; }
    void setCompanyName(const QString &name) { m_companyName = name; }
    void setAddress(const QString &address) { m_address = address; }
    void setLegalInfo(const QString &info) { m_legalInfo = info; }

private:
    static QString toRtfString(const QString &value);

    QString m_logoPath;
    QString m_companyName;
    QString m_address;
    QString m_legalInfo;
};

#endif // WORDREPORTEXPORTER_H
