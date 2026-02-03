#ifndef CHECKPDFEXPORTER_H
#define CHECKPDFEXPORTER_H

#include <QString>
#include <memory>

class EasyPOSCore;

class CheckPdfExporter
{
public:
    static bool saveToPdf(std::shared_ptr<EasyPOSCore> core, qint64 checkId, const QString &path);
};

#endif // CHECKPDFEXPORTER_H
