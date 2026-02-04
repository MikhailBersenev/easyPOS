#ifndef PRODUCTIONRUNDIALOG_H
#define PRODUCTIONRUNDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui { class ProductionRunDialog; }
class EasyPOSCore;

class ProductionRunDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductionRunDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~ProductionRunDialog();

private slots:
    void onRecipeSelected(int index);
    void onRunClicked();

private:
    void loadRecipes();
    void updateComponentsInfo();

    Ui::ProductionRunDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
};

#endif // PRODUCTIONRUNDIALOG_H
