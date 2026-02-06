#ifndef POSITIONSDIALOG_H
#define POSITIONSDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;
struct Position;

/**
 * @brief Справочник должностей
 */
class PositionsDialog : public ReferenceDialog
{
    Q_OBJECT
public:
    explicit PositionsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~PositionsDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditPosition(const Position *existing = nullptr);
};

#endif // POSITIONSDIALOG_H
