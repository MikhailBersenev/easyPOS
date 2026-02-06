#include "positioneditdialog.h"
#include "ui_positioneditdialog.h"
#include <QMessageBox>

PositionEditDialog::PositionEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PositionEditDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (name().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите название должности."));
            ui->nameEdit->setFocus();
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    ui->nameEdit->setFocus();
}

PositionEditDialog::~PositionEditDialog()
{
    delete ui;
}

void PositionEditDialog::setData(const QString &name, bool isActive)
{
    ui->nameEdit->setText(name);
    ui->activeCheckBox->setChecked(isActive);
}

QString PositionEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

bool PositionEditDialog::isActive() const
{
    return ui->activeCheckBox->isChecked();
}
