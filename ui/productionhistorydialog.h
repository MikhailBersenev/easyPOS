#ifndef PRODUCTIONHISTORYDIALOG_H
#define PRODUCTIONHISTORYDIALOG_H

#include <QDialog>
#include <memory>
#include "../production/structures.h"

namespace Ui { class ProductionHistoryDialog; }
class EasyPOSCore;

class ProductionHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductionHistoryDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~ProductionHistoryDialog();

private slots:
    void onDateRangeChanged();
    void refreshTable();
    void onReportWizard();

private:
    Ui::ProductionHistoryDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    QList<ProductionRun> m_lastRuns;
};

#endif // PRODUCTIONHISTORYDIALOG_H
