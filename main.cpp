#include "mainwindow.h"
#include "ui/authwindow.h"
#include "ui/setupwizard.h"
#include "easyposcore.h"
#include "logging/logmanager.h"
#include "settings/settingsmanager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QDebug>
#include <QSqlError>
#include <QSettings>
#include <QDir>
#include <memory>

namespace {
bool loadTranslation(QApplication &app, const QString &languageCode)
{
    static QTranslator translator;
    app.removeTranslator(&translator);
    QString baseName;
    if (languageCode == QLatin1String("ru"))
        baseName = QStringLiteral("easyPOS_ru_RU");
    else if (languageCode == QLatin1String("en"))
        baseName = QStringLiteral("easyPOS_en");
    else
        return false;
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList searchPaths = {
        QStringLiteral(":/i18n"),
        appDir + QStringLiteral("/i18n"),
        appDir,  // build/ при разработке (lrelease пишет .qm сюда)
        appDir + QStringLiteral("/../share/easyPOS/i18n"),
    };
    for (const QString &path : searchPaths) {
        if (translator.load(baseName, path)) {
            app.installTranslator(&translator);
            return true;
        }
    }
    return false;
}

void applyLanguage(QApplication &app)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                QCoreApplication::organizationName(),
                QCoreApplication::applicationName());
    QString lang = s.value(SettingsKeys::Language, QString()).toString();
    if (lang == QLatin1String("ru") || lang == QLatin1String("en")) {
        loadTranslation(app, lang);
    } else {
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale : uiLanguages) {
            QString name = QLocale(locale).name();
            if (name.startsWith(QLatin1String("ru"))) {
                if (loadTranslation(app, QStringLiteral("ru"))) break;
            } else if (name.startsWith(QLatin1String("en"))) {
                if (loadTranslation(app, QStringLiteral("en"))) break;
            }
        }
    }
}
} // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("easyPOS"));
    QCoreApplication::setApplicationName(QStringLiteral("easyPOS"));

    LogManager::init();
    qInfo() << "easyPOS started, log file:" << LogManager::logFilePath();

    applyLanguage(a);

    std::shared_ptr<EasyPOSCore> easyPOSCore = std::make_shared<EasyPOSCore>();

    // Проверяем, нужна ли первоначальная настройка
    if (SetupWizard::setupRequired(easyPOSCore)) {
        SetupWizard wizard(easyPOSCore);
        if (wizard.exec() != QDialog::Accepted)
            return 0;  // Пользователь закрыл мастер — выходим
    }

    easyPOSCore->ensureDbConnection();

    AuthWindow authWindow(nullptr, easyPOSCore);
    if (!authWindow.isSessionRestored())
        authWindow.show();
    return a.exec();
}
