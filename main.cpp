#include "mainwindow.h"
#include "ui/authwindow.h"
#include "ui/setupwizard.h"
#include "easyposcore.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QDebug>
#include <QSqlError>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("easyPOS"));
    QCoreApplication::setApplicationName(QStringLiteral("easyPOS"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "easyPOS_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

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
