#ifndef SIGNUPWINDOW_H
#define SIGNUPWINDOW_H

#include <QDialog>

#include "../easyposcore.h"
namespace Ui {
class SignupWindow;
}

class SignupWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SignupWindow(QWidget *parent = nullptr, std::shared_ptr<EasyPOSCore> easyPOSCore = nullptr);
    ~SignupWindow();

private slots:
    void on_signupButton_clicked();

    void on_showPasswordCheckBox_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::SignupWindow *ui;
    std::shared_ptr<EasyPOSCore> m_easyPOSCore;
    AccountManager* m_accountManager;
    RoleManager* m_roleManager;
    bool isInputValid();
};

#endif // SIGNUPWINDOW_H
