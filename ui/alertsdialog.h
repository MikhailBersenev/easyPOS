#ifndef ALERTSDIALOG_H
#define ALERTSDIALOG_H

#include <QDialog>
#include <memory>
#include "../alerts/structures.h"

namespace Ui { class AlertsDialog; }
class EasyPOSCore;

class AlertsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlertsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~AlertsDialog();

private slots:
    void onFilterChanged();
    void refreshTable();
    void onExportCsv();
    void onRowDoubleClicked();

private:
    void fillCategoryCombo();
    QString categoryFilterValue() const;

    Ui::AlertsDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    QList<Alert> m_lastAlerts;
};

#endif // ALERTSDIALOG_H
