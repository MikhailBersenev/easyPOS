#include "vatrateeditdialog.h"
#include "ui_vatrateeditdialog.h"
#include <QMessageBox>

VatRateEditDialog::VatRateEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VatRateEditDialog)
{
    ui->setupUi(this);
    ui->rateSpin->setRange(0, 100);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название."));
            ui->nameEdit->setFocus();
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->nameEdit->setFocus();
}

VatRateEditDialog::~VatRateEditDialog()
{
    delete ui;
}

void VatRateEditDialog::setData(const QString &name, double rate)
{
    ui->nameEdit->setText(name);
    ui->rateSpin->setValue(rate);
}

QString VatRateEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

double VatRateEditDialog::rate() const
{
    return ui->rateSpin->value();
}
