#include "settingsmanager.h"
#include "../logging/logmanager.h"
#include <QSettings>
#include <QCoreApplication>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "SettingsManager::SettingsManager";
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                               QCoreApplication::organizationName(),
                               QCoreApplication::applicationName(),
                               this);
}

SettingsManager::~SettingsManager()
{
    qDebug() << "SettingsManager::~SettingsManager";
    m_settings->sync();
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    qDebug() << "SettingsManager::setValue" << "key=" << key;
    if (m_settings->value(key) != value) {
        qDebug() << "SettingsManager::setValue: branch changed";
        m_settings->setValue(key, value);
        emit settingsChanged();
    } else {
        qDebug() << "SettingsManager::setValue: branch unchanged";
    }
}

bool SettingsManager::contains(const QString &key) const
{
    return m_settings->contains(key);
}

void SettingsManager::remove(const QString &key)
{
    qDebug() << "SettingsManager::remove" << "key=" << key;
    if (m_settings->contains(key)) {
        qDebug() << "SettingsManager::remove: branch had_key";
        m_settings->remove(key);
        emit settingsChanged();
    } else {
        qDebug() << "SettingsManager::remove: branch no_key";
    }
}

void SettingsManager::sync()
{
    qDebug() << "SettingsManager::sync";
    m_settings->sync();
}

PostgreSQLAuth SettingsManager::loadDatabaseSettings() const
{
    qDebug() << "SettingsManager::loadDatabaseSettings";
    PostgreSQLAuth auth;
    auth.host = stringValue(SettingsKeys::DbHost, QStringLiteral("localhost"));
    auth.port = intValue(SettingsKeys::DbPort, 5432);
    auth.database = stringValue(SettingsKeys::DbName, QStringLiteral("pos_bakery"));
    auth.username = stringValue(SettingsKeys::DbUsername, QStringLiteral("postgres"));
    auth.password = stringValue(SettingsKeys::DbPassword);
    auth.sslMode = stringValue(SettingsKeys::DbSslMode, QStringLiteral("prefer"));
    auth.connectTimeout = intValue(SettingsKeys::DbConnectTimeout, 10);
    return auth;
}

void SettingsManager::saveDatabaseSettings(const PostgreSQLAuth &auth)
{
    qDebug() << "SettingsManager::saveDatabaseSettings" << "host=" << auth.host;
    m_settings->setValue(SettingsKeys::DbHost, auth.host);
    m_settings->setValue(SettingsKeys::DbPort, auth.port);
    m_settings->setValue(SettingsKeys::DbName, auth.database);
    m_settings->setValue(SettingsKeys::DbUsername, auth.username);
    m_settings->setValue(SettingsKeys::DbPassword, auth.password);
    m_settings->setValue(SettingsKeys::DbSslMode, auth.sslMode);
    m_settings->setValue(SettingsKeys::DbConnectTimeout, auth.connectTimeout);
    emit settingsChanged();
}

QString SettingsManager::stringValue(const QString &key, const QString &defaultValue) const
{
    return value(key, defaultValue).toString();
}

int SettingsManager::intValue(const QString &key, int defaultValue) const
{
    return value(key, defaultValue).toInt();
}

bool SettingsManager::boolValue(const QString &key, bool defaultValue) const
{
    return value(key, defaultValue).toBool();
}
