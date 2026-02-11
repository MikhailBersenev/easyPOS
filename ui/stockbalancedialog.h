#ifndef STOCKBALANCEDIALOG_H
#define STOCKBALANCEDIALOG_H

#include "referencedialog.h"

class StockBalanceDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit StockBalanceDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private slots:
    void onExportReport();

private:
    class StockManager *stockManager() const;
    QList<QPair<qint64, QString>> loadGoodsList() const;
};

#endif // STOCKBALANCEDIALOG_H
