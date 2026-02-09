#ifndef SETUPBRANDINGPAGE_H
#define SETUPBRANDINGPAGE_H

#include <QWizardPage>

namespace Ui {
class SetupBrandingPage;
}

/**
 * @brief Страница мастера настройки: брендирование (название, логотип, адрес, реквизиты)
 */
class SetupBrandingPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit SetupBrandingPage(QWidget *parent = nullptr);
    ~SetupBrandingPage();

    QString appName() const;
    QString logoPath() const;
    QString address() const;
    QString legalInfo() const;

private slots:
    void on_browseLogoButton_clicked();

private:
    Ui::SetupBrandingPage *ui;
};

#endif // SETUPBRANDINGPAGE_H
