#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QWizard>
#include <memory>

class EasyPOSCore;
struct PostgreSQLAuth;

/**
 * @brief Мастер первоначальной настройки приложения
 *
 * Показывается при первом запуске. Настраивает БД, брендирование и первого пользователя (Администратор).
 */
class SetupWizard : public QWizard
{
    Q_OBJECT

public:
    explicit SetupWizard(std::shared_ptr<EasyPOSCore> core, QWidget *parent = nullptr);
    ~SetupWizard();

    /**
     * @brief Нужно ли показывать мастер (настройка ещё не выполнена)
     */
    static bool setupRequired(std::shared_ptr<EasyPOSCore> core);

    void accept() override;

    int nextId() const override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onTestConnectionClicked();
    void onCurrentIdChanged(int id);

private:
    void setupPages();
    void saveSettings();
    bool saveBrandingAndAdmin();
    /** Есть ли в БД роль с level=0 и хотя бы один пользователь с этой ролью (пропуск страниц бренда и админа) */
    bool hasAdminRoleAndUser(const PostgreSQLAuth &auth) const;

    std::shared_ptr<EasyPOSCore> m_core;
    class SetupDbPage *m_dbPage = nullptr;
    class SetupBrandingPage *m_brandingPage = nullptr;
    class SetupAdminPage *m_adminPage = nullptr;
    int m_dbPageId = -1;
    int m_adminPageId = -1;
    int m_finishPageId = -1;
    mutable bool m_skipBrandingAndAdmin = false;  // выставляется в nextId() при переходе со страницы БД
    class QComboBox *m_languageCombo = nullptr;
};

#endif // SETUPWIZARD_H
