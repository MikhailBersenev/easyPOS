#ifndef SETUPWIZARD_H
#define SETUPWIZARD_H

#include <QWizard>
#include <memory>

class EasyPOSCore;

/**
 * @brief Мастер первоначальной настройки приложения
 *
 * Показывается при первом запуске. Настраивает подключение к базе данных.
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

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onTestConnectionClicked();
    void onCurrentIdChanged(int id);

private:
    void setupPages();
    void saveSettings();

    std::shared_ptr<EasyPOSCore> m_core;
    class SetupDbPage *m_dbPage = nullptr;
    int m_dbPageId = -1;
    class QComboBox *m_languageCombo = nullptr;
};

#endif // SETUPWIZARD_H
