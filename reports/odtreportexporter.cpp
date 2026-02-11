#include "odtreportexporter.h"
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QLocale>
#include <QTextStream>
#include <QStringConverter>

static QString escapeXml(const QString &value)
{
    QString out;
    out.reserve(value.size());
    for (const QChar c : value) {
        if (c == QLatin1Char('&'))
            out += QLatin1String("&amp;");
        else if (c == QLatin1Char('<'))
            out += QLatin1String("&lt;");
        else if (c == QLatin1Char('>'))
            out += QLatin1String("&gt;");
        else if (c == QLatin1Char('"'))
            out += QLatin1String("&quot;");
        else if (c.unicode() == '\'')
            out += QLatin1String("&apos;");
        else
            out += c;
    }
    return out;
}

QString OdtReportExporter::escapeXml(const QString &value)
{
    return ::escapeXml(value);
}

bool OdtReportExporter::exportReport(const ReportData &data, const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    QLocale loc;
    const QString nsOffice = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:office:1.0");
    const QString nsStyle = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:style:1.0");
    const QString nsText = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:text:1.0");
    const QString nsTable = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:table:1.0");
    const QString nsDraw = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
    const QString nsFo = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
    const QString nsSvg = QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<office:document xmlns:office=\"" << nsOffice << "\" xmlns:style=\"" << nsStyle
        << "\" xmlns:text=\"" << nsText << "\" xmlns:table=\"" << nsTable << "\" xmlns:draw=\"" << nsDraw
        << "\" xmlns:fo=\"" << nsFo << "\" xmlns:svg=\"" << nsSvg
        << "\" office:version=\"1.2\" office:mimetype=\"application/vnd.oasis.opendocument.text\">\n";

    out << "<office:styles>\n";
    out << "  <style:style style:name=\"ReportTitle\" style:family=\"paragraph\">\n";
    out << "    <style:text-properties fo:font-size=\"14pt\" fo:font-weight=\"bold\"/>\n";
    out << "    <style:paragraph-properties fo:text-align=\"center\" fo:margin-bottom=\"0.2cm\"/>\n";
    out << "  </style:style>\n";
    out << "  <style:style style:name=\"ReportHeading\" style:family=\"paragraph\">\n";
    out << "    <style:text-properties fo:font-size=\"12pt\" fo:font-weight=\"bold\"/>\n";
    out << "    <style:paragraph-properties fo:text-align=\"center\" fo:margin-bottom=\"0.15cm\"/>\n";
    out << "  </style:style>\n";
    out << "  <style:style style:name=\"ReportCenter\" style:family=\"paragraph\">\n";
    out << "    <style:paragraph-properties fo:text-align=\"center\" fo:margin-bottom=\"0.1cm\"/>\n";
    out << "  </style:style>\n";
    out << "  <style:style style:name=\"TableHeader\" style:family=\"paragraph\">\n";
    out << "    <style:text-properties fo:font-weight=\"bold\"/>\n";
    out << "  </style:style>\n";
    out << "</office:styles>\n";

    out << "<office:body>\n<office:text>\n";

    // Логотип (встроенный как base64)
    if (!m_logoPath.isEmpty()) {
        QFileInfo fi(m_logoPath);
        if (fi.exists() && fi.isFile()) {
            QImage img(m_logoPath);
            if (!img.isNull() && img.width() > 0 && img.height() > 0) {
                QByteArray ba;
                QBuffer buf(&ba);
                buf.open(QIODevice::WriteOnly);
                img.save(&buf, "PNG");
                QString b64 = QString::fromLatin1(ba.toBase64());
                double widthCm = qMin(8.0, img.width() * 2.54 / 96.0);
                double heightCm = img.height() * widthCm / img.width();
                out << "  <text:p text:style-name=\"ReportCenter\">\n";
                out << "    <draw:frame draw:name=\"Logo\" text:anchor-type=\"paragraph\" "
                    << "svg:width=\"" << QString::number(widthCm, 'f', 2) << "cm\" "
                    << "svg:height=\"" << QString::number(heightCm, 'f', 2) << "cm\">\n";
                out << "      <draw:image draw:name=\"logo.png\">\n";
                out << "        <office:binary-data>" << b64 << "</office:binary-data>\n";
                out << "      </draw:image>\n";
                out << "    </draw:frame>\n";
                out << "  </text:p>\n";
            }
        }
    }

    // Реквизиты предприятия
    if (!m_companyName.isEmpty()) {
        out << "  <text:p text:style-name=\"ReportTitle\">" << escapeXml(m_companyName) << "</text:p>\n";
    }
    if (!m_address.isEmpty()) {
        out << "  <text:p text:style-name=\"ReportCenter\">" << escapeXml(m_address) << "</text:p>\n";
    }
    if (!m_legalInfo.isEmpty()) {
        out << "  <text:p text:style-name=\"ReportCenter\">" << escapeXml(m_legalInfo) << "</text:p>\n";
    }
    out << "  <text:p/>\n";

    // Название отчёта
    if (!data.title.isEmpty()) {
        out << "  <text:p text:style-name=\"ReportHeading\">" << escapeXml(data.title) << "</text:p>\n";
    }
    if (data.generatedAt.isValid()) {
        out << "  <text:p text:style-name=\"ReportCenter\">"
            << escapeXml(loc.toString(data.generatedAt, QLocale::LongFormat)) << "</text:p>\n";
    }
    out << "  <text:p/>\n";

    // Сводные показатели
    if (!data.summaryLines.isEmpty()) {
        out << "  <text:p text:style-name=\"TableHeader\">" << escapeXml(QStringLiteral("Сводные показатели:")) << "</text:p>\n";
        for (const QString &line : data.summaryLines) {
            out << "  <text:p>" << escapeXml(line) << "</text:p>\n";
        }
        out << "  <text:p/>\n";
    }

    // Таблица
    if (!data.header.isEmpty()) {
        const int cols = data.header.size();
        out << "  <table:table table:name=\"ReportTable\">\n";
        for (int i = 0; i < cols; ++i)
            out << "    <table:table-column/>\n";

        out << "    <table:table-header-rows>\n      <table:table-row>\n";
        for (int c = 0; c < cols; ++c) {
            out << "        <table:table-cell office:value-type=\"string\">\n";
            out << "          <text:p text:style-name=\"TableHeader\">" << escapeXml(data.header.at(c)) << "</text:p>\n";
            out << "        </table:table-cell>\n";
        }
        out << "      </table:table-row>\n    </table:table-header-rows>\n";

        for (const QStringList &row : data.rows) {
            out << "    <table:table-row>\n";
            for (int c = 0; c < cols; ++c) {
                QString cell = (c < row.size()) ? row.at(c) : QString();
                out << "      <table:table-cell office:value-type=\"string\">\n";
                out << "        <text:p>" << escapeXml(cell) << "</text:p>\n";
                out << "      </table:table-cell>\n";
            }
            out << "    </table:table-row>\n";
        }
        out << "  </table:table>\n";
        out << "  <text:p/>\n";
    }

    // Подпись и печать
    out << "  <text:p>" << escapeXml(QStringLiteral("Составил отчёт: _______________________ (подпись)")) << "</text:p>\n";
    if (!data.preparedByName.isEmpty())
        out << "  <text:p>" << escapeXml(QStringLiteral("ФИО: ") + data.preparedByName) << "</text:p>\n";
    if (!data.preparedByPosition.isEmpty())
        out << "  <text:p>" << escapeXml(QStringLiteral("Должность: ") + data.preparedByPosition) << "</text:p>\n";
    out << "  <text:p>" << escapeXml(QStringLiteral("Печать (М.П.): _______________________")) << "</text:p>\n";

    out << "</office:text>\n</office:body>\n</office:document>\n";

    return true;
}

QString OdtReportExporter::formatDescription() const
{
    return QStringLiteral("ODT (OpenDocument Text)");
}

QString OdtReportExporter::defaultFileExtension() const
{
    return QStringLiteral("fodt");
}
