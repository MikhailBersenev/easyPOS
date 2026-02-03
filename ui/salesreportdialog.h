#ifndef SALESREPORTDIALOG_H
#define SALESREPORTDIALOG_H

#include <QDialog>
#include <memory>

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
    class SalesReportDialogPrivate *d;
};

#endif // SALESREPORTDIALOG_H
