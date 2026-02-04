#ifndef RECIPESDIALOG_H
#define RECIPESDIALOG_H

#include "referencedialog.h"

class RecipesDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit RecipesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    class ProductionManager *productionManager() const;
    QList<QPair<qint64, QString>> loadGoodsList() const;
    void addOrEditRecipe(qint64 existingId);
};

#endif // RECIPESDIALOG_H
