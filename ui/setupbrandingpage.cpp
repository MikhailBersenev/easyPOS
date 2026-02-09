#include "setupbrandingpage.h"
#include "ui_setupbrandingpage.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

SetupBrandingPage::SetupBrandingPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::SetupBrandingPage)
{
    ui->setupUi(this);
}

SetupBrandingPage::~SetupBrandingPage()
{
    delete ui;
}

QString SetupBrandingPage::appName() const
{
    QString s = ui->appNameEdit->text().trimmed();
    return s.isEmpty() ? QStringLiteral("easyPOS") : s;
}

QString SetupBrandingPage::logoPath() const
{
    return ui->logoPathEdit->text().trimmed();
}

QString SetupBrandingPage::address() const
{
    return ui->addressEdit->text().trimmed();
}

QString SetupBrandingPage::legalInfo() const
{
    return ui->legalInfoEdit->toPlainText().trimmed();
}

void SetupBrandingPage::on_browseLogoButton_clicked()
{
    QString current = ui->logoPathEdit->text().trimmed();
    QString dir = current.isEmpty() ? QDir::homePath() : QFileInfo(current).absolutePath();
    QString path = QFileDialog::getOpenFileName(this, tr("Выбрать логотип"), dir,
        tr("Изображения (*.png *.jpg *.jpeg *.bmp);;Все файлы (*)"));
    if (!path.isEmpty())
        ui->logoPathEdit->setText(QDir::toNativeSeparators(path));
}
