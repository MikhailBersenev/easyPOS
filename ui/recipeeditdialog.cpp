#include "recipeeditdialog.h"
#include "ui_recipeeditdialog.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>

RecipeEditDialog::RecipeEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RecipeEditDialog)
{
    ui->setupUi(this);
    ui->componentsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->componentsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(ui->addCompBtn, &QPushButton::clicked, this, &RecipeEditDialog::onAddComponent);
    connect(ui->removeCompBtn, &QPushButton::clicked, this, &RecipeEditDialog::onRemoveComponent);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (recipeName().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название рецепта."));
            ui->nameEdit->setFocus();
            return;
        }
        if (outputGoodId() <= 0) {
            QMessageBox::warning(this, windowTitle(), tr("Выберите продукт на выходе."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->nameEdit->setFocus();
}

RecipeEditDialog::~RecipeEditDialog()
{
    delete ui;
}

void RecipeEditDialog::setRecipe(const ProductionRecipe &recipe)
{
    ui->nameEdit->setText(recipe.name);
    ui->outputQtySpin->setValue(recipe.outputQuantity);
    fillOutputGoodCombo();
    int idx = -1;
    for (int i = 0; i < ui->outputGoodCombo->count(); ++i) {
        if (ui->outputGoodCombo->itemData(i).toLongLong() == recipe.outputGoodId) {
            idx = i;
            break;
        }
    }
    if (idx >= 0)
        ui->outputGoodCombo->setCurrentIndex(idx);
}

void RecipeEditDialog::setGoodsList(const QList<QPair<qint64, QString>> &goodIdsNames)
{
    m_goodsList = goodIdsNames;
    fillOutputGoodCombo();
    fillComponentGoodCombo();
}

void RecipeEditDialog::setComponents(const QList<RecipeComponent> &components)
{
    ui->componentsTable->setRowCount(0);
    for (const RecipeComponent &c : components) {
        const int row = ui->componentsTable->rowCount();
        ui->componentsTable->insertRow(row);
        ui->componentsTable->setItem(row, 0, new QTableWidgetItem(c.goodName));
        ui->componentsTable->item(row, 0)->setData(Qt::UserRole, c.goodId);
        ui->componentsTable->setItem(row, 1, new QTableWidgetItem(QString::number(c.quantity)));
    }
}

QString RecipeEditDialog::recipeName() const
{
    return ui->nameEdit->text().trimmed();
}

qint64 RecipeEditDialog::outputGoodId() const
{
    const int idx = ui->outputGoodCombo->currentIndex();
    if (idx < 0 || idx >= m_goodsList.size())
        return 0;
    return m_goodsList.at(idx).first;
}

double RecipeEditDialog::outputQuantity() const
{
    return ui->outputQtySpin->value();
}

QList<QPair<qint64, double>> RecipeEditDialog::components() const
{
    QList<QPair<qint64, double>> list;
    for (int row = 0; row < ui->componentsTable->rowCount(); ++row) {
        QTableWidgetItem *idItem = ui->componentsTable->item(row, 0);
        QTableWidgetItem *qtyItem = ui->componentsTable->item(row, 1);
        if (!idItem || !qtyItem) continue;
        qint64 goodId = idItem->data(Qt::UserRole).toLongLong();
        bool ok = false;
        double qty = qtyItem->text().replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
        if (goodId > 0 && ok && qty > 0)
            list.append({ goodId, qty });
    }
    return list;
}

void RecipeEditDialog::fillOutputGoodCombo()
{
    const qint64 current = outputGoodId();
    ui->outputGoodCombo->clear();
    for (const auto &p : m_goodsList) {
        ui->outputGoodCombo->addItem(p.second, p.first);
    }
    if (current > 0) {
        for (int i = 0; i < ui->outputGoodCombo->count(); ++i) {
            if (ui->outputGoodCombo->itemData(i).toLongLong() == current) {
                ui->outputGoodCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void RecipeEditDialog::fillComponentGoodCombo()
{
    ui->componentGoodCombo->clear();
    for (const auto &p : m_goodsList)
        ui->componentGoodCombo->addItem(p.second, p.first);
}

int RecipeEditDialog::componentGoodComboIndex() const
{
    const int idx = ui->componentGoodCombo->currentIndex();
    if (idx < 0 || idx >= m_goodsList.size())
        return -1;
    return idx;
}

void RecipeEditDialog::onAddComponent()
{
    const int idx = componentGoodComboIndex();
    if (idx < 0 || m_goodsList.isEmpty()) return;
    const qint64 goodId = m_goodsList.at(idx).first;
    const QString name = m_goodsList.at(idx).second;
    const double qty = ui->componentQtySpin->value();
    if (qty <= 0) return;

    const int row = ui->componentsTable->rowCount();
    ui->componentsTable->insertRow(row);
    ui->componentsTable->setItem(row, 0, new QTableWidgetItem(name));
    ui->componentsTable->item(row, 0)->setData(Qt::UserRole, goodId);
    ui->componentsTable->setItem(row, 1, new QTableWidgetItem(QString::number(qty)));
}

void RecipeEditDialog::onRemoveComponent()
{
    const int row = ui->componentsTable->currentRow();
    if (row >= 0)
        ui->componentsTable->removeRow(row);
}
