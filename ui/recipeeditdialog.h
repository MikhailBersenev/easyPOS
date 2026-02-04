#ifndef RECIPEEDITDIALOG_H
#define RECIPEEDITDIALOG_H

#include <QDialog>
#include "../production/structures.h"

namespace Ui { class RecipeEditDialog; }

class RecipeEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecipeEditDialog(QWidget *parent = nullptr);
    ~RecipeEditDialog();
    void setRecipe(const ProductionRecipe &recipe);
    void setGoodsList(const QList<QPair<qint64, QString>> &goodIdsNames);
    void setComponents(const QList<RecipeComponent> &components);

    QString recipeName() const;
    qint64 outputGoodId() const;
    double outputQuantity() const;
    QList<QPair<qint64, double>> components() const;

private slots:
    void onAddComponent();
    void onRemoveComponent();

private:
    void fillOutputGoodCombo();
    void fillComponentGoodCombo();
    int componentGoodComboIndex() const;

    Ui::RecipeEditDialog *ui;
    QList<QPair<qint64, QString>> m_goodsList;
};

#endif // RECIPEEDITDIALOG_H
