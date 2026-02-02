#ifndef SERVICESDIALOG_H
#define SERVICESDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;

/**
 * @brief Справочник услуг
 */
class ServicesDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit ServicesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~ServicesDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditService(int existingId = 0);
};

#endif // SERVICESDIALOG_H
