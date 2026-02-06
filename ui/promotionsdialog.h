#ifndef PROMOTIONSDIALOG_H
#define PROMOTIONSDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;
struct Promotion;

class PromotionsDialog : public ReferenceDialog
{
    Q_OBJECT
public:
    explicit PromotionsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~PromotionsDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditPromotion(const Promotion *existing);
};

#endif // PROMOTIONSDIALOG_H
