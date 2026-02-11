#include "pdfreportexporter.h"
#include <QPrinter>
#include <QPainter>
#include <QImage>
#include <QFileInfo>
#include <QLocale>
#include <QFontDatabase>

bool PdfReportExporter::exportReport(const ReportData &data, const QString &filePath)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QPainter painter;
    if (!painter.begin(&printer))
        return false;

    // Координаты в точках (1/72 дюйма), иначе при 300 DPI всё рисуется в углу и накладывается
    QRectF pageRectPt = printer.pageRect(QPrinter::Point);
    const double pageWidthPt = pageRectPt.width();
    const double pageHeightPt = pageRectPt.height();
    painter.setWindow(QRect(0, 0, int(pageWidthPt), int(pageHeightPt)));

    QLocale loc;
    const double marginPt = 42; // ~15 mm
    const double contentWidth = pageWidthPt - 2 * marginPt;
    double y = marginPt;
    const double lineSpacing = 14;
    const double blockSpacing = 20;

    // Шрифт с кириллицей, иначе в PDF получаются «квадраты» вместо букв
    QFont font;
    const QStringList tryFamilies = { QStringLiteral("DejaVu Sans"), QStringLiteral("Liberation Sans"),
                                      QStringLiteral("FreeSans"), QStringLiteral("Arial") };
    for (const QString &family : tryFamilies) {
        if (QFontDatabase().hasFamily(family)) {
            font.setFamily(family);
            break;
        }
    }
    font.setPointSize(10);
    painter.setFont(font);

    auto nextLine = [&](double extra = 0) { y += lineSpacing + extra; };

    // ----- Логотип -----
    if (!m_logoPath.isEmpty()) {
        QFileInfo fi(m_logoPath);
        if (fi.exists() && fi.isFile()) {
            QImage img(m_logoPath);
            if (!img.isNull()) {
                const double maxLogoW = contentWidth * 0.5; // макс. ширина в точках
                const double logoW = qMin(maxLogoW, double(img.width()) * 72.0 / 96.0); // пиксели → точки
                const double logoH = img.height() * logoW / img.width();
                const double xLogo = marginPt + (contentWidth - logoW) / 2;
                painter.drawImage(QRectF(xLogo, y, logoW, logoH), img);
                y += logoH + blockSpacing;
            }
        }
    }

    // ----- Реквизиты предприятия (по центру) -----
    if (!m_companyName.isEmpty()) {
        QFont boldFont = font;
        boldFont.setPointSize(14);
        boldFont.setBold(true);
        painter.setFont(boldFont);
        QRectF textRect(marginPt, y, contentWidth, 60);
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_companyName);
        painter.setFont(font);
        y = textRect.bottom() + lineSpacing;
    }
    if (!m_address.isEmpty()) {
        QRectF textRect(marginPt, y, contentWidth, 40);
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_address);
        y = textRect.bottom() + lineSpacing;
    }
    if (!m_legalInfo.isEmpty()) {
        font.setPointSize(8);
        painter.setFont(font);
        QRectF textRect(marginPt, y, contentWidth, 30);
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_legalInfo);
        font.setPointSize(10);
        painter.setFont(font);
        y = textRect.bottom() + blockSpacing;
    }

    // Линия
    painter.drawLine(QPointF(marginPt, y), QPointF(marginPt + contentWidth, y));
    nextLine(blockSpacing);

    // ----- Название отчёта -----
    if (!data.title.isEmpty()) {
        QFont titleFont = font;
        titleFont.setPointSize(12);
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.drawText(QRectF(marginPt, y, contentWidth, 30), Qt::AlignCenter, data.title);
        painter.setFont(font);
        nextLine();
    }
    if (data.generatedAt.isValid()) {
        painter.drawText(QRectF(marginPt, y, contentWidth, 20), Qt::AlignCenter, loc.toString(data.generatedAt, QLocale::LongFormat));
        nextLine(blockSpacing);
    }

    // ----- Сводные показатели -----
    if (!data.summaryLines.isEmpty()) {
        QFont boldFont = font;
        boldFont.setBold(true);
        painter.setFont(boldFont);
        painter.drawText(marginPt, y, QStringLiteral("Сводные показатели:"));
        nextLine();
        painter.setFont(font);
        for (const QString &line : data.summaryLines) {
            painter.drawText(marginPt, y, line);
            nextLine();
        }
        nextLine(blockSpacing);
        painter.drawLine(QPointF(marginPt, y), QPointF(marginPt + contentWidth, y));
        nextLine(blockSpacing);
    }

    // ----- Таблица -----
    if (!data.header.isEmpty()) {
        const int cols = data.header.size();
        QList<double> colWidths;
        double colW = contentWidth / cols;
        for (int i = 0; i < cols; ++i)
            colWidths.append(colW);

        const double rowHeight = 22;
        const double headerHeight = 26;

        auto drawTableRow = [&](const QStringList &cells, bool isHeader) {
            if (y + (isHeader ? headerHeight : rowHeight) > pageHeightPt - marginPt - 80) {
                printer.newPage();
                y = marginPt;
            }
            double x = marginPt;
            for (int c = 0; c < cols; ++c) {
                double w = colWidths.at(c);
                QRectF cellRect(x, y, w, isHeader ? headerHeight : rowHeight);
                painter.drawRect(cellRect);
                QString text = c < cells.size() ? cells.at(c) : QString();
                if (text.length() > 25)
                    text = text.left(22) + QStringLiteral("…");
                painter.drawText(cellRect.adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text);
                x += w;
            }
            y += isHeader ? headerHeight : rowHeight;
        };

        QFont headerFont = font;
        headerFont.setBold(true);
        painter.setFont(headerFont);
        drawTableRow(data.header, true);
        painter.setFont(font);

        for (const QStringList &row : data.rows)
            drawTableRow(row, false);

        nextLine(blockSpacing);
    }

    // ----- Подпись и печать -----
    if (y > pageHeightPt - marginPt - 120) {
        printer.newPage();
        y = marginPt;
    }
    painter.drawLine(QPointF(marginPt, y), QPointF(marginPt + contentWidth, y));
    nextLine(blockSpacing);

    painter.drawText(marginPt, y, QStringLiteral("Составил отчёт: _______________________ (подпись)"));
    nextLine();
    if (!data.preparedByName.isEmpty())
        painter.drawText(marginPt, y, QStringLiteral("ФИО: ") + data.preparedByName);
    nextLine();
    if (!data.preparedByPosition.isEmpty())
        painter.drawText(marginPt, y, QStringLiteral("Должность: ") + data.preparedByPosition);
    nextLine();
    painter.drawText(marginPt, y, QStringLiteral("Печать (М.П.): _______________________"));

    painter.end();
    return true;
}

QString PdfReportExporter::formatDescription() const
{
    return QStringLiteral("PDF (Portable Document Format)");
}

QString PdfReportExporter::defaultFileExtension() const
{
    return QStringLiteral("pdf");
}
