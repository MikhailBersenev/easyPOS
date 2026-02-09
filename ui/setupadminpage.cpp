#include "setupadminpage.h"
#include "ui_setupadminpage.h"
#include <QLineEdit>

SetupAdminPage::SetupAdminPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::SetupAdminPage)
{
    ui->setupUi(this);
    connect(ui->usernameEdit, &QLineEdit::textChanged, this, &SetupAdminPage::updateComplete);
    connect(ui->passwordEdit, &QLineEdit::textChanged, this, &SetupAdminPage::updateComplete);
    connect(ui->repeatPasswordEdit, &QLineEdit::textChanged, this, &SetupAdminPage::updateComplete);
}

SetupAdminPage::~SetupAdminPage()
{
    delete ui;
}

QString SetupAdminPage::username() const
{
    return ui->usernameEdit->text().trimmed();
}

QString SetupAdminPage::password() const
{
    return ui->passwordEdit->text();
}

bool SetupAdminPage::isComplete() const
{
    const QString u = ui->usernameEdit->text().trimmed();
    const QString p = ui->passwordEdit->text();
    const QString r = ui->repeatPasswordEdit->text();
    if (u.isEmpty() || p.isEmpty() || r.isEmpty())
        return false;
    if (p != r)
        return false;
    if (p.length() < 8)
        return false;
    bool hasUpper = false;
    bool onlyDigits = true;
    for (const QChar &c : p) {
        if (c.isLetter() && c.isUpper())
            hasUpper = true;
        if (c.isLetter())
            onlyDigits = false;
    }
    if (!hasUpper || onlyDigits)
        return false;
    return true;
}

void SetupAdminPage::on_showPasswordCheckBox_toggled(bool checked)
{
    auto mode = checked ? QLineEdit::Normal : QLineEdit::Password;
    ui->passwordEdit->setEchoMode(mode);
    ui->repeatPasswordEdit->setEchoMode(mode);
}

void SetupAdminPage::updateComplete()
{
    emit completeChanged();
}
