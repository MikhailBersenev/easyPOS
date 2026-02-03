#ifndef CHECKHISTORYDIALOG_H
#define CHECKHISTORYDIALOG_H

#include <QDialog>
#include <memory>

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
    class CheckHistoryDialogPrivate *d;
};

#endif // CHECKHISTORYDIALOG_H
