#include "logmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStringConverter>
#include <QCoreApplication>
#include <QSettings>
#include <QRegularExpression>
#include <iostream>

LogManager *LogManager::s_instance = nullptr;
bool LogManager::s_consoleOutput = true;

static QString defaultLogDirectory()
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!logDir.isEmpty())
        return logDir + QStringLiteral("/easyPOS/logs");
    return QCoreApplication::applicationDirPath() + QStringLiteral("/logs");
}

/** Имя файла лога: easypos_YYYY-MM-DD_HH-mm-ss_<host>.txt. При каждом запуске — новый файл. */
static QString makeLogFileName(const QString &serverHost)
{
    QString date = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));
    QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH-mm-ss"));
    QString host = serverHost.trimmed().isEmpty() ? QStringLiteral("local") : serverHost;
    // Убираем символы, недопустимые в имени файла
    host.replace(QRegularExpression(QStringLiteral("[/\\\\:*?\"<>|]")), QStringLiteral("_"));
    return QStringLiteral("easypos_%1_%2_%3.txt").arg(date, time, host);
}

LogManager::LogManager(QObject *parent)
    : QObject(parent)
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                QCoreApplication::organizationName(),
                QCoreApplication::applicationName());
    QString logDir = s.value(QStringLiteral("logging/path"), QString()).toString().trimmed();
    if (logDir.isEmpty())
        logDir = defaultLogDirectory();

    QDir().mkpath(logDir);
    QString serverHost = s.value(QStringLiteral("database/host"), QString()).toString().trimmed();
    if (serverHost.isEmpty())
        serverHost = QStringLiteral("localhost");
    QString path = logDir + QLatin1Char('/') + makeLogFileName(serverHost);

    m_logFile.setFileName(path);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
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
    if (!s_instance || !s_instance->m_logFile.isOpen())
        return QString();
    return s_instance->m_logFile.fileName();
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
