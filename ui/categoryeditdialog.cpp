#include "categoryeditdialog.h"
#include "ui_categoryeditdialog.h"
#include <QMessageBox>

CategoryEditDialog::CategoryEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CategoryEditDialog)
{
    ui->setupUi(this);
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

CategoryEditDialog::~CategoryEditDialog()
{
    delete ui;
}

void CategoryEditDialog::setData(const QString &name, const QString &description)
{
    ui->nameEdit->setText(name);
    ui->descriptionEdit->setPlainText(description);
}

QString CategoryEditDialog::name() const
{
    return ui->nameEdit->text().trimmed();
}

QString CategoryEditDialog::description() const
{
    return ui->descriptionEdit->toPlainText().trimmed();
}
