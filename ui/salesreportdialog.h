#ifndef SALESREPORTDIALOG_H
#define SALESREPORTDIALOG_H

#include <QDialog>
#include <memory>
#include "../sales/structures.h"

namespace Ui { class SalesReportDialog; }
class EasyPOSCore;

class SalesReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SalesReportDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~SalesReportDialog();

private slots:
    void onGenerate();
    void onExportCsv();

private:
    Ui::SalesReportDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    QList<Check> m_lastChecks;
};

#endif // SALESREPORTDIALOG_H
