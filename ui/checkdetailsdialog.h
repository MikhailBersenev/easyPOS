#ifndef CHECKDETAILSDIALOG_H
#define CHECKDETAILSDIALOG_H

#include <QDialog>
#include <memory>

class EasyPOSCore;

class CheckDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CheckDetailsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core, qint64 checkId);
    ~CheckDetailsDialog();

private slots:
    void onSaveToPdf();

private:
    class CheckDetailsDialogPrivate *d;
};

#endif // CHECKDETAILSDIALOG_H
