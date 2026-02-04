#ifndef CHECKDETAILSDIALOG_H
#define CHECKDETAILSDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui { class CheckDetailsDialog; }
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
    Ui::CheckDetailsDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    qint64 m_checkId;
};

#endif // CHECKDETAILSDIALOG_H
