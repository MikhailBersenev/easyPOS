#include "mainwindow.h"
#include "ui/authwindow.h"
#include "ui/setupwizard.h"
#include "easyposcore.h"
#include "logging/logmanager.h"
#include "settings/settingsmanager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QDebug>
#include <QSqlError>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>
#include <memory>
#include "translationutils.h"

// Глобальная функция для загрузки переводов (доступна из других файлов)
bool loadApplicationTranslation(const QString &languageCode)
{
    QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app)
        return false;
    
    static QTranslator *translator = nullptr;
    if (!translator) {
        translator = new QTranslator(app);
    }
    app->removeTranslator(translator);
    
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
        if (translator->load(baseName, path)) {
            app->installTranslator(translator);
            // Отправляем событие LanguageChange для всех окон
            QEvent ev(QEvent::LanguageChange);
            for (QWidget *widget : app->allWidgets()) {
                if (widget)
                    QCoreApplication::sendEvent(widget, &ev);
            }
            return true;
        }
    }
    return false;
}

namespace {
bool loadTranslation(QApplication &app, const QString &languageCode)
{
    return loadApplicationTranslation(languageCode);
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

static QString comboArrowImagePath()
{
    /* Ресурс :/ в QSS иногда не подхватывается для QComboBox::down-arrow на части платформ.
     * Копируем PNG во временный файл и подставляем file:// путь в QSS. */
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QDir dir(cacheDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return QString();
    const QString path = dir.absoluteFilePath(QStringLiteral("easypos_arrow_down.png"));
    if (QFile::exists(path))
        return path;
    if (QFile::copy(QStringLiteral(":/icons/arrow_down.png"), path))
        return path;
    return QString();
}

bool loadLiquidGlassStyle(QApplication &app, bool dark)
{
    const QString resourcePath = dark ? QStringLiteral(":/styles/green_dark.qss")
                                      : QStringLiteral(":/styles/green_light.qss");
    const QString fileName = dark ? QStringLiteral("green_dark.qss")
                                  : QStringLiteral("green_light.qss");
    QString sheet;
    QFile res(resourcePath);
    if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sheet = QString::fromUtf8(res.readAll());
    } else {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QDir cwd(QDir::currentPath());
        const QStringList stylePaths = {
            QStringLiteral("/usr/share/easyPOS/styles/") + fileName,
            appDir + QStringLiteral("/styles/") + fileName,
            appDir + QStringLiteral("/../styles/") + fileName,
            appDir + QStringLiteral("/../../styles/") + fileName,
            cwd.absoluteFilePath(QStringLiteral("styles/") + fileName),
        };
        for (const QString &path : stylePaths) {
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                sheet = QString::fromUtf8(f.readAll());
                break;
            }
        }
    }
    if (sheet.isEmpty())
        return false;
    const QString arrowPath = comboArrowImagePath();
    if (!arrowPath.isEmpty()) {
        const QString fileUrl = QUrl::fromLocalFile(arrowPath).toString();
        sheet.replace(QStringLiteral("url(:/icons/arrow_down.png)"), QStringLiteral("url(") + fileUrl + QLatin1Char(')'));
    }
    app.setStyleSheet(sheet);
    return true;
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

    /* Применяем зелёную тему: светлую при светлой ОС, тёмную при тёмной ОС */
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool dark = (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    loadLiquidGlassStyle(a, dark);
#else
    loadLiquidGlassStyle(a, false);
#endif

    std::shared_ptr<EasyPOSCore> easyPOSCore = std::make_shared<EasyPOSCore>();
    qInfo() << "EasyPOSCore created";

    // Проверяем, нужна ли первоначальная настройка
    if (SetupWizard::setupRequired(easyPOSCore)) {
        qInfo() << "Setup required, showing SetupWizard";
        SetupWizard wizard(easyPOSCore);
        if (wizard.exec() != QDialog::Accepted) {
            qInfo() << "SetupWizard cancelled, exiting";
            return 0;  // Пользователь закрыл мастер — выходим
        }
        qInfo() << "SetupWizard completed";
    }

    easyPOSCore->ensureDbConnection();
    qInfo() << "DB connection ensured";

    AuthWindow authWindow(nullptr, easyPOSCore);
    if (!authWindow.isSessionRestored())
        authWindow.show();
    else
        qInfo() << "Session restored, main window shown";
    return a.exec();
}
