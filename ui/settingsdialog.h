#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui { class SettingsDialog; }
class EasyPOSCore;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~SettingsDialog();

private slots:
    void onDbSettingsClicked();
    void on_logPathBrowseButton_clicked();
    void onBrandingLogoBrowseClicked();
    void onRolesAddClicked();
    void onRolesEditClicked();
    void onRolesDeleteClicked();
    void onUsersAddClicked();
    void onUsersEditClicked();
    void onUsersDeleteClicked();
    void onUsersRestoreClicked();
    void accept() override;

private:
    void loadFromSettings();
    void updateDbLabel();
    void loadRolesTable();
    void loadUsersTable();

    Ui::SettingsDialog *ui;
    std::shared_ptr<EasyPOSCore> m_core;
    class AccountManager *m_accountManager = nullptr;
    class RoleManager *m_roleManager = nullptr;
};

#endif // SETTINGSDIALOG_H
