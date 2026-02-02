#include "authwindow.h"
#include "ui_authwindow.h"
#include "../easyposcore.h"
#include "../RBAC/authmanager.h"
#include "../RBAC/structures.h"
#include "../settings/settingsmanager.h"
#include "windowcreator.h"
#include "signupwindow.h"
#include "../mainwindow.h"
#include <QMessageBox>
#include <QDebug>

AuthWindow::AuthWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> easyPOSCore)
    : QDialog(parent)
    , ui(new Ui::AuthWindow)
    , m_easyPOSCore(easyPOSCore)
    , m_authManager(nullptr)
{
    ui->setupUi(this);
    
    // Создаем AuthManager через фабричный метод
    if (m_easyPOSCore) {
        m_authManager = m_easyPOSCore->createAuthManager(this);
        connect(m_easyPOSCore.get(), &EasyPOSCore::sessionInvalidated, this, [this] { show(); });
        tryRestoreSession();
    }
}

bool AuthWindow::tryRestoreSession()
{
    if (!m_easyPOSCore || !m_authManager) return false;

    auto *sm = m_easyPOSCore->getSettingsManager();
    if (!sm || !sm->contains(SettingsKeys::SessionToken)) return false;

    QString token = sm->stringValue(SettingsKeys::SessionToken);
    if (token.isEmpty()) return false;

    UserSession session = m_authManager->getSessionByToken(token);
    if (!session.isValid() || session.isExpired()) {
        sm->remove(SettingsKeys::SessionToken);
        sm->sync();
        return false;
    }

    m_easyPOSCore->setCurrentSession(session);
    MainWindow *mw = WindowCreator::Create<MainWindow>(this, m_easyPOSCore);
    if (mw) {
        m_sessionRestored = true;
        hide();
        return true;
    }
    return false;
}

AuthWindow::~AuthWindow()
{
    // AuthManager будет удален автоматически, так как this является его родителем
    delete ui;
}

void AuthWindow::on_signInButton_clicked()
{
    if (!m_authManager) {
        QMessageBox::critical(this, "Ошибка", "Менеджер авторизации не инициализирован");
        return;
    }

    // Авторизация через AuthManager
    AuthResult result = m_authManager->authenticate(ui->usernameEdit->text(), ui->passwordEdit->text());
    if (result.success) {
        UserSession session = result.session;
        qDebug() << "Авторизован пользователь:" << session.username;
        qDebug() << "Роль:" << session.role.name;
        
        // Сохраняем сессию в EasyPOSCore
        if (m_easyPOSCore) {
            m_easyPOSCore->setCurrentSession(session);
            // Сохраняем токен для восстановления сессии
            if (auto *sm = m_easyPOSCore->getSettingsManager()) {
                sm->setValue(SettingsKeys::SessionToken, session.sessionToken);
                sm->sync();
            }
        }
        
#if 0
        // Проверка прав
        if (m_authManager->hasPermission(session.userId, "products.create")) {
            // Пользователь может создавать продукты
        }
#endif
        //QMessageBox::information(this, "Авторизация", result.message);
        MainWindow *mw = WindowCreator::Create<MainWindow>(this, m_easyPOSCore);
        if (mw) {
            hide();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось создать главное окно. Нет активной сессии.");
        }
    }
    else {
        QMessageBox::critical(this, "Ошибка", result.message);
    }
}


void AuthWindow::on_signUpButton_clicked()
{
    WindowCreator::Create<SignupWindow>(this, m_easyPOSCore);
}


void AuthWindow::on_showPasswordCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    switch(arg1) {
    case Qt::Checked:
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        return;
    case Qt::Unchecked:
        ui->passwordEdit->setEchoMode(QLineEdit::Password);
        return;
    case Qt::PartiallyChecked:
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        return;
    }
}

