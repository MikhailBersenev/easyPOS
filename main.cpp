#include "mainwindow.h"
#include "ui/authwindow.h"


#include <QApplication>
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

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "easyPOS_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    std::shared_ptr<EasyPOSCore> easyPOSCore = std::make_shared <EasyPOSCore>();
    AuthWindow authWindow(nullptr, easyPOSCore);
    authWindow.show();
    return a.exec();
}
