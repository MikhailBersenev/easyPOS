#include "signupwindow.h"
#include "ui_signupwindow.h"
#include "../RBAC/accountmanager.h"
#include "../RBAC/rolemanager.h"
#include "../RBAC/structures.h"
#include "../easyposcore.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"

#include <QMessageBox>

SignupWindow::SignupWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> easyPOSCore)
    : QDialog(parent)
    , ui(new Ui::SignupWindow)
    , m_easyPOSCore(easyPOSCore)
{
    ui->setupUi(this);
    m_accountManager = m_easyPOSCore->createAccountManager(this);
    m_roleManager = m_easyPOSCore->createRoleManager(this);
}

SignupWindow::~SignupWindow()
{
    delete ui;
}

void SignupWindow::on_signupButton_clicked()
{
    if(isInputValid() && !m_accountManager->userExists(ui->usernameEdit->text())) {
        UserOperationResult result;
        Role pendingUsersrole;
        if(m_roleManager->roleExists("Неподтвержденные пользователи")) {
            pendingUsersrole = m_roleManager->getRole("Неподтвержденные пользователи");
            result = m_accountManager->registerUser(ui->usernameEdit->text(), ui->passwordEdit->text(), pendingUsersrole.id);
        }
        RoleOperationResult roleResult = m_roleManager->createRole("Неподтвержденные пользователи", 99);
        if(roleResult.success)
            result = m_accountManager->registerUser(ui->usernameEdit->text(), ui->passwordEdit->text(), roleResult.roleId);
        if (result.success && m_easyPOSCore) {
            if (auto *alerts = m_easyPOSCore->createAlertsManager())
                alerts->log(AlertCategory::Reference, AlertSignature::UserCreated,
                            tr("Самостоятельная регистрация: %1").arg(ui->usernameEdit->text()),
                            result.userId, m_easyPOSCore->getEmployeeIdByUserId(result.userId));
        }
        QMessageBox::information(this, "Регистрация", result.message);
    }
    else {
        QMessageBox::critical(this, "Ошибка", "Проверьте корректность введенных данных");
    }
    close();
}

bool SignupWindow::isInputValid()
{
    const QString username = ui->usernameEdit->text();
    const QString password = ui->passwordEdit->text();
    const QString passwordRepeat = ui->repeatPasswordEdit->text();

    if(username.isEmpty() || password.isEmpty() || passwordRepeat.isEmpty())
        return false;
    if(password != passwordRepeat)
        return false;

    bool ok=false;
    QString AlphaBet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if(password.length()>=8) //Не меньше 8 символов
    {
        if(!password.toFloat()) { //цифры буквы
            for (int i=0; i < password.length(); i++ ) {

                for (int counter=0;counter<25;counter++ ) {
                    if(password[i] == AlphaBet[counter]) {
                        ok = true; //если есть заглавная ставим ок
                        break;
                    }
                }
                if(ok==true) {
                    break; //если ок то прерываем весь цикл
                }
            }
            if(ok==true) {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}


void SignupWindow::on_showPasswordCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    switch(arg1) {
    case Qt::Checked:
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        ui->repeatPasswordEdit->setEchoMode(QLineEdit::Normal);
        return;
    case Qt::Unchecked:
        ui->passwordEdit->setEchoMode(QLineEdit::Password);
        ui->repeatPasswordEdit->setEchoMode(QLineEdit::Password);
        return;
    case Qt::PartiallyChecked:
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        ui->repeatPasswordEdit->setEchoMode(QLineEdit::Normal);
        return;
    }
}

