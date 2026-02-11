#ifndef CHECKEXPORTER_H
#define CHECKEXPORTER_H

#include <QString>
#include <QList>
#include <memory>

class EasyPOSCore;
class SalesManager;
struct Check;
struct SaleRow;

class CheckExporter
{
public:
    // Генерация текстового представления чека
    static QString generateText(std::shared_ptr<EasyPOSCore> core, SalesManager *salesManager,
                                qint64 checkId, const Check &ch, const QList<SaleRow> &rows, double toPay);
    
    // Сохранение чека в txt файл
    static bool saveToTxt(std::shared_ptr<EasyPOSCore> core, SalesManager *salesManager,
                          qint64 checkId, const QString &filePath);
};

#endif // CHECKEXPORTER_H

