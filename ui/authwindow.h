#ifndef AUTHWINDOW_H
#define AUTHWINDOW_H

#include "../easyposcore.h"
#include "../RBAC/structures.h"
#include <QDialog>
#include <memory>

// Предварительное объявление
class AuthManager;

namespace Ui {
class AuthWindow;
}

class AuthWindow : public QDialog
{
    Q_OBJECT

public:
     AuthWindow(QWidget *parent = nullptr, std::shared_ptr<EasyPOSCore> easyPOSCore = nullptr);
    ~AuthWindow();
    
    // Получение текущей сессии
    UserSession getCurrentSession() const { return m_currentSession; }
    
    // Получение менеджера авторизации
    AuthManager* getAuthManager() const { return m_authManager; }

    /**
     * @brief Сессия восстановлена по токену (открыт MainWindow)
     */
    bool isSessionRestored() const { return m_sessionRestored; }

private slots:
    void on_badgeReturnPressed();
    void on_signInButton_clicked();
    void on_signUpButton_clicked();
    void on_showPasswordCheckBox_checkStateChanged(const Qt::CheckState &arg1);

private:
    bool tryRestoreSession();
    Ui::AuthWindow *ui;
    bool m_sessionRestored = false;
    std::shared_ptr<EasyPOSCore> m_easyPOSCore;
    AuthManager* m_authManager;
    UserSession m_currentSession;
};

#endif // AUTHWINDOW_H
