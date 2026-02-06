#include "authwindow.h"
#include "ui_authwindow.h"
#include "../easyposcore.h"
#include "../RBAC/authmanager.h"
#include "../RBAC/structures.h"
#include "../settings/settingsmanager.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include "../logging/logmanager.h"
#include "windowcreator.h"
#include "signupwindow.h"
#include "../mainwindow.h"
#include <QMessageBox>

AuthWindow::AuthWindow(QWidget *parent, std::shared_ptr<EasyPOSCore> easyPOSCore)
    : QDialog(parent)
    , ui(new Ui::AuthWindow)
    , m_easyPOSCore(easyPOSCore)
    , m_authManager(nullptr)
{
    ui->setupUi(this);
    connect(ui->badgeLineEdit, &QLineEdit::returnPressed, this, &AuthWindow::on_badgeReturnPressed);
    connect(ui->badgeSignInButton, &QPushButton::clicked, this, &AuthWindow::on_badgeReturnPressed);

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
        qInfo() << "AuthWindow: session restore failed (invalid or expired token)";
        sm->remove(SettingsKeys::SessionToken);
        sm->sync();
        return false;
    }

    m_easyPOSCore->setCurrentSession(session);
    qInfo() << "AuthWindow: session restored, user=" << session.username;
    MainWindow *mw = WindowCreator::Create<MainWindow>(this, m_easyPOSCore);
    if (mw) {
        m_sessionRestored = true;
        hide();
        return true;
    }
    qWarning() << "AuthWindow: failed to create MainWindow after session restore";
    return false;
}

AuthWindow::~AuthWindow()
{
    // AuthManager будет удален автоматически, так как this является его родителем
    delete ui;
}

void AuthWindow::on_badgeReturnPressed()
{
    const QString code = ui->badgeLineEdit->text().trimmed();
    ui->badgeLineEdit->clear();
    if (code.isEmpty()) return;
    if (!m_authManager) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Менеджер авторизации не инициализирован"));
        return;
    }
    AuthResult result = m_authManager->authenticateByBadgeBarcode(code);
    if (result.success) {
        if (m_easyPOSCore) {
            m_easyPOSCore->setCurrentSession(result.session);
            if (auto *sm = m_easyPOSCore->getSettingsManager()) {
                sm->setValue(SettingsKeys::SessionToken, result.session.sessionToken);
                sm->sync();
            }
        }
        qInfo() << "AuthWindow: badge login success, user=" << result.session.username;
        if (auto *alerts = m_easyPOSCore->createAlertsManager()) {
            qint64 empId = m_easyPOSCore->getEmployeeIdByUserId(result.session.userId);
            alerts->log(AlertCategory::Auth, AlertSignature::LoginSuccess,
                        QStringLiteral("User %1 logged in (badge)").arg(result.session.username),
                        result.session.userId, empId);
        }
        MainWindow *mw = WindowCreator::Create<MainWindow>(this, m_easyPOSCore);
        if (mw) hide();
        else {
            qWarning() << "AuthWindow: failed to create MainWindow after badge login";
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать главное окно."));
        }
    } else {
        qWarning() << "AuthWindow: badge login failed:" << result.message;
        QMessageBox::warning(this, tr("Вход по бейджу"), result.message);
    }
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
        qInfo() << "AuthWindow: login success, user=" << session.username << "role=" << session.role.name;
        
        // Сохраняем сессию в EasyPOSCore
        if (m_easyPOSCore) {
            m_easyPOSCore->setCurrentSession(session);
            // Сохраняем токен для восстановления сессии
            if (auto *sm = m_easyPOSCore->getSettingsManager()) {
                sm->setValue(SettingsKeys::SessionToken, session.sessionToken);
                sm->sync();
            }
            if (auto *alerts = m_easyPOSCore->createAlertsManager()) {
                qint64 empId = m_easyPOSCore->getEmployeeIdByUserId(session.userId);
                alerts->log(AlertCategory::Auth, AlertSignature::LoginSuccess,
                            QStringLiteral("User %1 logged in").arg(session.username),
                            session.userId, empId);
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
            qWarning() << "AuthWindow: failed to create MainWindow after login";
            QMessageBox::critical(this, "Ошибка", "Не удалось создать главное окно. Нет активной сессии.");
        }
    }
    else {
        qWarning() << "AuthWindow: login failed, user=" << ui->usernameEdit->text() << "msg=" << result.message;
        if (auto *alerts = m_easyPOSCore->createAlertsManager()) {
            alerts->log(AlertCategory::Auth, AlertSignature::LoginFailed,
                        QStringLiteral("Login failed: %1 — %2").arg(ui->usernameEdit->text(), result.message));
        }
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

