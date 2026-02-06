#include "checkpdfexporter.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QPrinter>
#include <QPainter>
#include <QLocale>
#include <QCoreApplication>

static void renderCheckContent(QPainter &painter, qint64 checkId, const Check &ch,
                               const QList<SaleRow> &rows, double toPay, SalesManager *sm)
{
    const int lineHeight = 24;
    int y = 50;
    const int leftMargin = 50;
    auto tr = [](const char *s) { return QCoreApplication::translate("CheckPdfExporter", s); };

    painter.drawText(leftMargin, y, tr("easyPOS — Чек №%1").arg(checkId));
    y += lineHeight * 2;
    painter.drawText(leftMargin, y, QLocale::system().toString(ch.date, QLocale::ShortFormat) + QLatin1String(" ") + ch.time.toString(Qt::ISODate));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Сотрудник: %1").arg(ch.employeeName));
    y += lineHeight * 2;

    painter.drawText(leftMargin, y, tr("Товар"));
    painter.drawText(leftMargin + 250, y, tr("Кол-во"));
    painter.drawText(leftMargin + 320, y, tr("Цена"));
    painter.drawText(leftMargin + 400, y, tr("Сумма"));
    painter.drawText(leftMargin + 480, y, tr("НДС ₽"));
    y += lineHeight;

    double totalVat = 0.0;
    for (const SaleRow &r : rows) {
        const double rate = sm ? sm->getVatRatePercent(r.vatRateId) : 0.0;
        const double vatAmt = rate > 0 ? r.sum * rate / (100.0 + rate) : 0.0;
        totalVat += vatAmt;
        const double priceToShow = r.originalUnitPrice > 0.0 ? r.originalUnitPrice : r.unitPrice;
        painter.drawText(leftMargin, y, r.itemName.left(35));
        painter.drawText(leftMargin + 250, y, QString::number(r.qnt));
        painter.drawText(leftMargin + 320, y, QString::number(priceToShow, 'f', 2));
        painter.drawText(leftMargin + 400, y, QString::number(r.sum, 'f', 2));
        painter.drawText(leftMargin + 480, y, vatAmt > 0 ? QString::number(vatAmt, 'f', 2) : QLatin1String("—"));
        y += lineHeight;
    }
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Итого: %1 ₽").arg(QString::number(ch.totalAmount, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Скидка: %1 ₽").arg(QString::number(ch.discountAmount, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("Сумма НДС: %1 ₽").arg(QString::number(totalVat, 'f', 2)));
    y += lineHeight;
    painter.drawText(leftMargin, y, tr("К оплате: %1 ₽").arg(QString::number(toPay, 'f', 2)));
}

bool CheckPdfExporter::saveToPdf(std::shared_ptr<EasyPOSCore> core, qint64 checkId, const QString &path)
{
    if (!core || checkId <= 0 || path.isEmpty()) return false;
    auto *sm = core->createSalesManager(nullptr);
    if (!sm) return false;
    const Check ch = sm->getCheck(checkId);
    if (ch.id == 0) return false;
    const QList<SaleRow> rows = sm->getSalesByCheck(checkId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);

    QPainter painter;
    if (!painter.begin(&printer)) return false;
    renderCheckContent(painter, checkId, ch, rows, toPay, sm);
    painter.end();
    return true;
}
