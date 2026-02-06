#include "promotioneditdialog.h"
#include "ui_promotioneditdialog.h"
#include <QMessageBox>

PromotionEditDialog::PromotionEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PromotionEditDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название акции."));
            ui->nameEdit->setFocus();
            return;
        }
        if (percent() <= 0 && sum() <= 0) {
            QMessageBox::warning(this, windowTitle(), tr("Укажите скидку в процентах или суммой."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->nameEdit->setFocus();
}

PromotionEditDialog::~PromotionEditDialog()
{
    delete ui;
}

void PromotionEditDialog::setData(const QString &name, const QString &description, double percentVal, double sumVal)
{
    ui->nameEdit->setText(name);
    ui->descriptionEdit->setText(description);
    ui->percentSpin->setValue(percentVal);
    ui->sumSpin->setValue(sumVal);
}

QString PromotionEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

QString PromotionEditDialog::description() const
{
    return ui->descriptionEdit->text().trimmed();
}

double PromotionEditDialog::percent() const
{
    return ui->percentSpin->value();
}

double PromotionEditDialog::sum() const
{
    return ui->sumSpin->value();
}
