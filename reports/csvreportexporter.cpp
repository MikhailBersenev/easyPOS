#include "csvreportexporter.h"
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

bool CsvReportExporter::exportReport(const ReportData &data, const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    const char sep = ';';

    if (!data.header.isEmpty()) {
        for (int i = 0; i < data.header.size(); ++i) {
            if (i > 0) out << sep;
            out << escapeCsvField(data.header.at(i));
        }
        out << "\n";
    }

    for (const QStringList &row : data.rows) {
        for (int i = 0; i < row.size(); ++i) {
            if (i > 0) out << sep;
            out << escapeCsvField(i < row.size() ? row.at(i) : QString());
        }
        out << "\n";
    }

    return true;
}

QString CsvReportExporter::formatDescription() const
{
    return QStringLiteral("CSV (UTF-8, разделитель — точка с запятой)");
}

QString CsvReportExporter::defaultFileExtension() const
{
    return QStringLiteral("csv");
}

QString CsvReportExporter::escapeCsvField(const QString &value)
{
    if (value.contains(QLatin1Char('"')) || value.contains(QLatin1Char(';')) || value.contains(QLatin1Char('\n'))) {
        return QLatin1Char('"') + QString(value).replace(QLatin1String("\""), QLatin1String("\"\"")) + QLatin1Char('"');
    }
    return value;
}
