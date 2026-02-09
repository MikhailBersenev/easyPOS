#ifndef SETUPADMINPAGE_H
#define SETUPADMINPAGE_H

#include <QWizardPage>

namespace Ui {
class SetupAdminPage;
}

/**
 * @brief Страница мастера настройки: первый пользователь (Администратор) и роли
 */
class SetupAdminPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit SetupAdminPage(QWidget *parent = nullptr);
    ~SetupAdminPage();

    QString username() const;
    QString password() const;
    bool isComplete() const override;

private slots:
    void on_showPasswordCheckBox_toggled(bool checked);
    void updateComplete();

private:
    Ui::SetupAdminPage *ui;
};

#endif // SETUPADMINPAGE_H
