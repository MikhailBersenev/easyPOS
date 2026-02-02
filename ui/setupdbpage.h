#ifndef SETUPDBPAGE_H
#define SETUPDBPAGE_H

#include <QWizardPage>
#include "../db/structures.h"

namespace Ui {
class SetupDbPage;
}

/**
 * @brief Страница мастера настройки: параметры подключения к БД
 */
class SetupDbPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit SetupDbPage(QWidget *parent = nullptr);
    ~SetupDbPage();

    PostgreSQLAuth getAuth() const;
    void setResult(const QString &text, bool success);

    void loadFromSettings(class SettingsManager *sm);
    void setResultStyle(const QString &styleSheet);

signals:
    void testConnectionRequested();

private slots:
    void on_testButton_clicked();

private:
    Ui::SetupDbPage *ui;
};

#endif // SETUPDBPAGE_H
