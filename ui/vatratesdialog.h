#ifndef VATRATESDIALOG_H
#define VATRATESDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;

/**
 * @brief Справочник ставок НДС
 */
class VatRatesDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit VatRatesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~VatRatesDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditVatRate(int existingId = 0);
};

#endif // VATRATESDIALOG_H
