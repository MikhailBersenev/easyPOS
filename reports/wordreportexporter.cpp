#include "wordreportexporter.h"
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QImage>
#include <QFileInfo>
#include <QLocale>

// RTF: экранирование и вывод текста в Unicode (\uN?)
static QString rtfEscape(const QString &value)
{
    QString out;
    out.reserve(value.size() * 2);
    for (const QChar c : value) {
        ushort u = c.unicode();
        if (u < 128) {
            if (c == QLatin1Char('\\') || c == QLatin1Char('{') || c == QLatin1Char('}'))
                out += QLatin1Char('\\');
            out += c;
        } else {
            // RTF Unicode: \uN? (N — signed 16-bit, ? — замена для старых читателей)
            int n = u;
            if (n > 32767)
                n -= 65536;
            out += QStringLiteral("\\u%1?").arg(n);
        }
    }
    return out;
}

QString WordReportExporter::toRtfString(const QString &value)
{
    return rtfEscape(value);
}

static bool writeRtfLogo(QTextStream &out, const QString &logoPath)
{
    QFileInfo fi(logoPath);
    if (!fi.exists() || !fi.isFile()) return false;
    QImage img(logoPath);
    if (img.isNull()) return false;
    QFile logoFile(logoPath);
    if (!logoFile.open(QIODevice::ReadOnly)) return false;
    QByteArray pngData = logoFile.readAll();
    logoFile.close();
    if (pngData.size() > 500000) return false; // ограничение размера
    int w = img.width(), h = img.height();
    if (w <= 0 || h <= 0) return false;
    // 96 DPI: 1 px ≈ 15 twips. Ограничиваем ширину 2.5 дюйма (3600 twips)
    const int maxTwips = 3600;
    int picw = w * 15, pich = h * 15;
    if (picw > maxTwips) {
        pich = pich * maxTwips / picw;
        picw = maxTwips;
    }
    out << "{\\pict\\pngblip\\picwgoal" << picw << "\\pichgoal" << pich << "\n";
    QString hex = QString::fromLatin1(pngData.toHex());
    for (int i = 0; i < hex.size(); i += 128)
        out << hex.mid(i, 128) << "\n";
    out << "}\\par\n";
    return true;
}

bool WordReportExporter::exportReport(const ReportData &data, const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    QLocale loc;

    out << "{\\rtf1\\ansi\\ansicpg1251\\deff0\n"
        << "{\\fonttbl{\\f0\\fswiss Arial;}}\n"
        << "\\viewkind4\\uc1\\pard\\f0\\fs24\n";

    // ----- Блок предприятия -----
    out << "\\pard\\qc\\sb120\\sa60 ";
    if (!m_logoPath.isEmpty())
        writeRtfLogo(out, m_logoPath);
    if (!m_companyName.isEmpty()) {
        out << "\\fs28\\b " << rtfEscape(m_companyName) << "\\b0\\fs24\\par\n";
    }
    if (!m_address.isEmpty()) {
        out << "\\sa60 " << rtfEscape(m_address) << "\\par\n";
    }
    if (!m_legalInfo.isEmpty()) {
        out << "\\fs20 " << rtfEscape(m_legalInfo) << "\\fs24\\par\n";
    }
    out << "\\par\n";
    // Линия под шапкой
    out << "\\pard\\qc\\brdrt\\brdrs\\brdrw10\\brsp80 ";
    out << "\\par\n\\par\n";

    // ----- Название отчёта -----
    out << "\\pard\\qc\\sb40\\sa20 ";
    if (!data.title.isEmpty()) {
        out << "\\fs26\\b " << rtfEscape(data.title) << "\\b0\\fs24\\par\n";
    }
    if (data.generatedAt.isValid()) {
        out << "\\fs22 " << rtfEscape(loc.toString(data.generatedAt, QLocale::LongFormat)) << "\\fs24\\par\n";
    }
    out << "\\par\n";

    // ----- Сводные показатели -----
    if (!data.summaryLines.isEmpty()) {
        out << "\\pard\\fi0\\li0\\sb20\\sa10 ";
        out << "\\b " << rtfEscape(QStringLiteral("Сводные показатели:")) << "\\b0\\par\n";
        for (const QString &line : data.summaryLines) {
            out << rtfEscape(line) << "\\par\n";
        }
        out << "\\par\n";
        out << "\\brdrt\\brdrs\\brdrw10\\brsp40 ";
        out << "\\par\n\\par\n";
    }

    // ----- Таблица -----
    if (!data.header.isEmpty()) {
        const int colTwips = 2000;
        QString rowDef;
        for (int i = 0; i < data.header.size(); ++i) {
            rowDef += QStringLiteral("\\clbrdrt\\brdrs\\clbrdrl\\brdrs\\clbrdrb\\brdrs\\clbrdrr\\brdrs\\cellx%1 ").arg((i + 1) * colTwips);
        }

        out << "\\trowd\\trgaph108\\trleft-108 " << rowDef << "\n";
        out << "\\intbl ";
        for (int i = 0; i < data.header.size(); ++i) {
            out << "\\b " << rtfEscape(data.header.at(i)) << "\\b0\\cell ";
        }
        out << "\\row\n";

        for (const QStringList &row : data.rows) {
            out << "\\trowd\\trgaph108\\trleft-108 " << rowDef << "\n\\intbl ";
            for (int i = 0; i < data.header.size(); ++i) {
                QString cell = (i < row.size()) ? row.at(i) : QString();
                out << rtfEscape(cell) << "\\cell ";
            }
            out << "\\row\n";
        }
        out << "\\pard\\par\n";
    }

    // ----- Блок подписи и печати -----
    out << "\\par\n\\par\n";
    out << "\\brdrt\\brdrs\\brdrw10\\brsp80 ";
    out << "\\par\n";
    out << "\\pard\\fi0\\li0\\sa100 ";
    out << rtfEscape(QStringLiteral("Составил отчёт:")) << " _______________________ ";
    out << rtfEscape(QStringLiteral("(подпись)")) << "\\par\n";
    if (!data.preparedByName.isEmpty()) {
        out << rtfEscape(QStringLiteral("ФИО:")) << " " << rtfEscape(data.preparedByName) << "\\par\n";
    }
    if (!data.preparedByPosition.isEmpty()) {
        out << rtfEscape(QStringLiteral("Должность:")) << " " << rtfEscape(data.preparedByPosition) << "\\par\n";
    }
    out << "\\par\n";
    out << rtfEscape(QStringLiteral("Печать (М.П.):")) << " _______________________ \\par\n";

    out << "}\n";
    return true;
}

QString WordReportExporter::formatDescription() const
{
    return QStringLiteral("Word / RTF (Rich Text Format)");
}

QString WordReportExporter::defaultFileExtension() const
{
    return QStringLiteral("rtf");
}
