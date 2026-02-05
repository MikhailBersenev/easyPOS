#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QVariant>
#include <QString>

#include "../db/structures.h"

class QSettings;

/**
 * @brief Менеджер настроек на базе QSettings
 * 
 * Хранит настройки приложения в платформенно-зависимом хранилище:
 * - Windows: реестр
 * - macOS: ~/Library/Preferences/
 * - Linux: ~/.config/<org>/<app>.conf
 */
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager();

    // --- Универсальные методы ---

    /**
     * @brief Получить значение настройки
     */
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    /**
     * @brief Установить значение настройки
     */
    void setValue(const QString &key, const QVariant &value);

    /**
     * @brief Проверить наличие ключа
     */
    bool contains(const QString &key) const;

    /**
     * @brief Удалить настройку
     */
    void remove(const QString &key);

    /**
     * @brief Синхронизировать изменения с диском
     */
    void sync();

    // --- Настройки подключения к БД ---

    PostgreSQLAuth loadDatabaseSettings() const;
    void saveDatabaseSettings(const PostgreSQLAuth &auth);

    // --- Типизированные геттеры ---

    QString stringValue(const QString &key, const QString &defaultValue = QString()) const;
    int intValue(const QString &key, int defaultValue = 0) const;
    bool boolValue(const QString &key, bool defaultValue = false) const;

signals:
    void settingsChanged();

private:
    QSettings *m_settings = nullptr;
};

// Константы ключей настроек
namespace SettingsKeys {
    // Язык интерфейса: "" = по системе, "ru" = русский, "en" = English
    constexpr const char *Language = "ui/language";

    // Первоначальная настройка
    constexpr const char *SetupCompleted = "setup/completed";

    // База данных
    constexpr const char *DbHost         = "database/host";
    constexpr const char *DbPort         = "database/port";
    constexpr const char *DbName         = "database/name";
    constexpr const char *DbUsername     = "database/username";
    constexpr const char *DbPassword     = "database/password";
    constexpr const char *DbSslMode      = "database/sslMode";
    constexpr const char *DbConnectTimeout = "database/connectTimeout";

    // Сессия (токен для восстановления)
    constexpr const char *SessionToken = "session/token";

    // Печать чека после оплаты
    constexpr const char *PrintAfterPay = "pos/printAfterPay";

    // Путь к файлу логов (пустой = по умолчанию, применится после перезапуска)
    constexpr const char *LogPath = "logging/path";

    // Геометрия главного окна
    constexpr const char *MainWindowGeometry = "mainwindow/geometry";
}

#endif // SETTINGSMANAGER_H
