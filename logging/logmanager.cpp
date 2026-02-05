#include "logmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QStringConverter>
#include <QCoreApplication>
#include <iostream>

LogManager *LogManager::s_instance = nullptr;
bool LogManager::s_consoleOutput = true;

LogManager::LogManager(QObject *parent)
    : QObject(parent)
{
    // ~/.local/share/easyPOS/logs (без дублирования имени приложения)
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!logDir.isEmpty()) {
        logDir += QStringLiteral("/easyPOS/logs");
    } else {
        logDir = QCoreApplication::applicationDirPath() + QStringLiteral("/logs");
    }
    QDir().mkpath(logDir);

    QString path = logDir + QStringLiteral("/easypos.txt");
    m_logFile.setFileName(path);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        m_stream.setEncoding(QStringConverter::Utf8);
    }
}

LogManager *LogManager::instance()
{
    if (!s_instance)
        s_instance = new LogManager(qApp);
    return s_instance;
}

void LogManager::init()
{
    instance();
    qInstallMessageHandler(LogManager::messageHandler);
}

void LogManager::shutdown()
{
    qInstallMessageHandler(nullptr);
}

QString LogManager::logFilePath()
{
    if (!s_instance)
        return QString();
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!logDir.isEmpty())
        return logDir + QStringLiteral("/easyPOS/logs/easypos.txt");
    return QCoreApplication::applicationDirPath() + QStringLiteral("/logs/easypos.txt");
}

void LogManager::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString level;
    switch (type) {
    case QtDebugMsg:    level = QStringLiteral("DEBUG"); break;
    case QtInfoMsg:     level = QStringLiteral("INFO"); break;
    case QtWarningMsg:  level = QStringLiteral("WARN"); break;
    case QtCriticalMsg: level = QStringLiteral("CRIT"); break;
    case QtFatalMsg:    level = QStringLiteral("FATAL"); break;
    }

    if (s_instance)
        s_instance->writeToFile(level, msg, context);

    if (s_consoleOutput) {
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type) {
        case QtDebugMsg:
            std::cerr << "[" << level.toLocal8Bit().constData() << "] " << localMsg.constData() << std::endl;
            break;
        case QtInfoMsg:
            std::cerr << "[" << level.toLocal8Bit().constData() << "] " << localMsg.constData() << std::endl;
            break;
        case QtWarningMsg:
            std::cerr << "[" << level.toLocal8Bit().constData() << "] " << localMsg.constData() << std::endl;
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            std::cerr << "[" << level.toLocal8Bit().constData() << "] " << localMsg.constData() << std::endl;
            break;
        }
    }

    if (type == QtFatalMsg)
        abort();
}

void LogManager::writeToFile(const QString &level, const QString &msg, const QMessageLogContext &context)
{
    if (!m_logFile.isOpen())
        return;
    QMutexLocker lock(&m_mutex);
    QString ts = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_stream << ts << " [" << level << "] " << msg;
    if (context.file && context.line > 0)
        m_stream << " (" << context.file << ":" << context.line << ")";
    m_stream << "\n";
    m_stream.flush();
}
