#ifndef GOODSDIALOG_H
#define GOODSDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;

/**
 * @brief Справочник товаров
 */
class GoodsDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit GoodsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~GoodsDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditGood(int existingId);
};

#endif // GOODSDIALOG_H
