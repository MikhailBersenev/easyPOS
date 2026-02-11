#include "checkexporter.h"
#include "../easyposcore.h"
#include "../sales/salesmanager.h"
#include "../sales/structures.h"
#include <QTextStream>
#include <QFile>
#include <QStringConverter>
#include <QLocale>
#include <QCoreApplication>

QString CheckExporter::generateText(std::shared_ptr<EasyPOSCore> core, SalesManager *salesManager,
                                     qint64 checkId, const Check &ch, const QList<SaleRow> &rows, double toPay)
{
    auto tr = [](const char *s) { return QCoreApplication::translate("CheckExporter", s); };
    
    QString text;
    QTextStream stream(&text);

    QString appName = core ? core->getBrandingAppName() : QStringLiteral("easyPOS");
    QString address = core ? core->getBrandingAddress() : QString();
    QString legalInfo = core ? core->getBrandingLegalInfo() : QString();
    
    const int lineWidth = 60; // Ширина строки для центрирования
    
    // Верхний разделитель
    stream << QString(lineWidth, QChar('=')) << "\n";
    
    // Заголовок (центрированный)
    QString header = QString("%1 — Чек №%2").arg(appName).arg(checkId);
    int headerPadding = (lineWidth - header.length()) / 2;
    if (headerPadding > 0) {
        stream << QString(headerPadding, QChar(' ')) << header << "\n";
    } else {
        stream << header << "\n";
    }
    
    stream << QString(lineWidth, QChar('=')) << "\n\n";
    
    // Дата и время
    QString dateTime = QLocale::system().toString(ch.date, QLocale::ShortFormat) + " " + ch.time.toString(Qt::ISODate);
    stream << dateTime << "\n";
    stream << tr("Сотрудник: %1").arg(ch.employeeName) << "\n";
    stream << QString(lineWidth, QChar('─')) << "\n\n";
    
    // Заголовок таблицы товаров
    stream << tr("Товар").leftJustified(30);
    stream << tr("Кол-во").rightJustified(8);
    stream << tr("Цена").rightJustified(10);
    stream << tr("Сумма").rightJustified(12) << "\n";
    stream << QString(lineWidth, QChar('─')) << "\n";

    // Список товаров
    for (const SaleRow &r : rows) {
        const QString vatName = salesManager ? salesManager->getVatRateName(r.vatRateId) : QString();
        const double priceToShow = r.originalUnitPrice > 0.0 ? r.originalUnitPrice : r.unitPrice;
        
        // Название товара (может быть многострочным)
        QString itemName = r.itemName;
        QStringList nameLines;
        if (itemName.length() > 30) {
            // Разбиваем длинные названия на несколько строк
            int pos = 0;
            while (pos < itemName.length()) {
                nameLines << itemName.mid(pos, 30);
                pos += 30;
            }
        } else {
            nameLines << itemName;
        }
        
        // Первая строка с данными
        stream << nameLines.first().leftJustified(30);
        stream << QString::number(r.qnt).rightJustified(8);
        stream << QString::number(priceToShow, 'f', 2).rightJustified(10);
        stream << QString::number(r.sum, 'f', 2).rightJustified(12);
        if (!vatName.isEmpty()) {
            stream << " (" << vatName << ")";
        }
        stream << "\n";
        
        // Дополнительные строки названия (если нужно)
        for (int i = 1; i < nameLines.size(); ++i) {
            stream << nameLines[i].leftJustified(30) << "\n";
        }
    }

    // Итоги
    stream << QString(lineWidth, QChar('─')) << "\n";
    stream << tr("Итого:").leftJustified(30) << QString::number(ch.totalAmount, 'f', 2).rightJustified(30) << " ₽\n";
    
    if (ch.discountAmount > 0.0) {
        stream << tr("Скидка:").leftJustified(30) << QString::number(ch.discountAmount, 'f', 2).rightJustified(30) << " ₽\n";
    }
    
    stream << QString(lineWidth, QChar('=')) << "\n";
    stream << tr("К ОПЛАТЕ:").leftJustified(30) << QString::number(toPay, 'f', 2).rightJustified(30) << " ₽\n";
    stream << QString(lineWidth, QChar('=')) << "\n\n";

    // Адрес и реквизиты
    if (!address.isEmpty() || !legalInfo.isEmpty()) {
        stream << QString(lineWidth, QChar('─')) << "\n";
        if (!address.isEmpty()) {
            stream << address << "\n";
        }
        if (!legalInfo.isEmpty()) {
            for (const QString &line : legalInfo.split(QLatin1Char('\n'))) {
                if (!line.trimmed().isEmpty()) {
                    stream << line.trimmed() << "\n";
                }
            }
        }
        stream << QString(lineWidth, QChar('─')) << "\n";
    }
    
    // Нижний разделитель
    stream << "\n" << QString(lineWidth, QChar('=')) << "\n";
    stream << tr("Спасибо за покупку!") << "\n";
    stream << QString(lineWidth, QChar('=')) << "\n";

    return text;
}

bool CheckExporter::saveToTxt(std::shared_ptr<EasyPOSCore> core, SalesManager *salesManager,
                               qint64 checkId, const QString &filePath)
{
    if (!salesManager || checkId <= 0 || filePath.isEmpty()) return false;
    const Check ch = salesManager->getCheck(checkId);
    if (ch.id == 0) return false;
    const QList<SaleRow> rows = salesManager->getSalesByCheck(checkId);
    const double toPay = (ch.totalAmount - ch.discountAmount) < 0 ? 0 : (ch.totalAmount - ch.discountAmount);

    QString text = generateText(core, salesManager, checkId, ch, rows, toPay);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << text;
    file.close();

    return true;
}

