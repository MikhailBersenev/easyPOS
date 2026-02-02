#ifndef GOODCATSDIALOG_H
#define GOODCATSDIALOG_H

#include "referencedialog.h"
#include <memory>

class EasyPOSCore;
class GoodCategory;

/**
 * @brief Справочник категорий товаров
 */
class GoodCatsDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit GoodCatsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~GoodCatsDialog();

protected:
    // Переопределение виртуальных методов базового класса
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;

private:
    void addOrEditCategory(const GoodCategory *existing = nullptr);
};

#endif // GOODCATSDIALOG_H
