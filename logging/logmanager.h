#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QtCore/qlogging.h>

/**
 * Менеджер логирования: перехватывает qDebug/qInfo/qWarning/qCritical
 * и записывает в текстовый файл с меткой времени и уровнем.
 *
 * Использование:
 *   #include "logging/logmanager.h"
 *   LOG_DEBUG << "сообщение" << value;
 *   LOG_INFO << "инфо";
 *   LOG_WARNING << "предупреждение";
 *   LOG_CRITICAL << "ошибка";
 *
 * Или напрямую: qDebug() << "текст";  — тоже попадёт в файл.
 */
class LogManager : public QObject
{
    Q_OBJECT
public:
    /** Инициализация: открыть файл логов и установить обработчик сообщений Qt */
    static void init();
    /** Отключить запись в файл и восстановить стандартный обработчик */
    static void shutdown();

    /** Путь к текущему файлу логов */
    static QString logFilePath();
    /** Включить/выключить вывод в консоль (по умолчанию true) */
    static void setConsoleOutput(bool enable) { s_consoleOutput = enable; }
    static bool consoleOutput() { return s_consoleOutput; }

    /** Вызывается из обработчика сообщений Qt (не вызывать вручную) */
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    explicit LogManager(QObject *parent = nullptr);
    static LogManager *instance();
    void writeToFile(const QString &level, const QString &msg, const QMessageLogContext &context);

    static LogManager *s_instance;
    static bool s_consoleOutput;
    QFile m_logFile;
    QTextStream m_stream;
    QMutex m_mutex;
};

// Макросы для единообразного логирования (всё идёт в файл через перехватчик)
#define LOG_DEBUG    qDebug
#define LOG_INFO     qInfo
#define LOG_WARNING  qWarning
#define LOG_CRITICAL qCritical

#endif // LOGMANAGER_H
