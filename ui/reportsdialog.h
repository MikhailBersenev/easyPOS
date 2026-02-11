#ifndef REPORTSDIALOG_H
#define REPORTSDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui { class ReportsDialog; }
class EasyPOSCore;

class ReportsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReportsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~ReportsDialog();

private slots:
    void onReportTypeChanged(int index);
    void onGenerate();
    void onExportCsv();
    void onExportOdt();
    void refreshShiftList();

private:
    void updateParamsVisibility();
    void fillPreviewFromReportData(const class ReportData &data);

    Ui::ReportsDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
};

#endif // REPORTSDIALOG_H
