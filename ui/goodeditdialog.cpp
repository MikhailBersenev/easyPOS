#include "goodeditdialog.h"
#include "ui_goodeditdialog.h"
#include <QMessageBox>

GoodEditDialog::GoodEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GoodEditDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название."));
            ui->nameEdit->setFocus();
            return;
        }
        if (m_categoryIds.isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Нет категорий. Сначала добавьте категории товаров."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->nameEdit->setFocus();
}

GoodEditDialog::~GoodEditDialog()
{
    delete ui;
}

void GoodEditDialog::setCategories(const QList<int> &ids, const QStringList &names)
{
    m_categoryIds = ids;
    ui->categoryCombo->clear();
    ui->categoryCombo->addItems(names);
}

void GoodEditDialog::setData(const QString &name, const QString &description, int categoryId, bool isActive)
{
    ui->nameEdit->setText(name);
    ui->descriptionEdit->setPlainText(description);
    ui->activeCheck->setChecked(isActive);
    int idx = m_categoryIds.indexOf(categoryId);
    if (idx >= 0)
        ui->categoryCombo->setCurrentIndex(idx);
}

QString GoodEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

QString GoodEditDialog::description() const
{
    return ui->descriptionEdit->toPlainText().trimmed();
}

int GoodEditDialog::categoryId() const
{
    int idx = ui->categoryCombo->currentIndex();
    if (idx >= 0 && idx < m_categoryIds.size())
        return m_categoryIds[idx];
    return m_categoryIds.value(0, 0);
}

bool GoodEditDialog::isActive() const
{
    return ui->activeCheck->isChecked();
}
