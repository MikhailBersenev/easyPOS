#ifndef CHECKHISTORYDIALOG_H
#define CHECKHISTORYDIALOG_H

#include <QDialog>
#include <memory>
#include "../sales/structures.h"

namespace Ui { class CheckHistoryDialog; }
class EasyPOSCore;

class CheckHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckHistoryDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~CheckHistoryDialog();

private slots:
    void onDateChanged();
    void refreshTable();
    void onExportCsv();

private:
    Ui::CheckHistoryDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    QList<Check> m_lastChecks;
};

#endif // CHECKHISTORYDIALOG_H
