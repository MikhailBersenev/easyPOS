#include "dbsettingsdialog.h"
#include "ui_dbsettingsdialog.h"
#include "../easyposcore.h"
#include "../settings/settingsmanager.h"
#include "../logging/logmanager.h"
#include "../db/databaseconnection.h"
#include "../db/structures.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include <QMessageBox>

DbSettingsDialog::DbSettingsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::DbSettingsDialog)
    , m_core(core)
{
    ui->setupUi(this);
    loadFromSettings();
    setResult(QString(), true);
}

DbSettingsDialog::~DbSettingsDialog()
{
    delete ui;
}

void DbSettingsDialog::loadFromSettings()
{
    if (!m_core) return;
    auto *sm = m_core->getSettingsManager();
    if (!sm) return;
    ui->hostEdit->setText(sm->stringValue(SettingsKeys::DbHost, QStringLiteral("localhost")));
    ui->portEdit->setText(QString::number(sm->intValue(SettingsKeys::DbPort, 5432)));
    ui->databaseEdit->setText(sm->stringValue(SettingsKeys::DbName, QStringLiteral("pos_bakery")));
    ui->usernameEdit->setText(sm->stringValue(SettingsKeys::DbUsername, QStringLiteral("postgres")));
    ui->passwordEdit->setText(sm->stringValue(SettingsKeys::DbPassword));
}

void DbSettingsDialog::saveToSettings()
{
    if (!m_core) return;
    auto *sm = m_core->getSettingsManager();
    if (!sm) return;
    PostgreSQLAuth auth;
    auth.host = ui->hostEdit->text().trimmed();
    if (auth.host.isEmpty()) auth.host = QStringLiteral("localhost");
    auth.port = ui->portEdit->text().trimmed().toInt();
    if (auth.port <= 0) auth.port = 5432;
    auth.database = ui->databaseEdit->text().trimmed();
    auth.username = ui->usernameEdit->text().trimmed();
    auth.password = ui->passwordEdit->text();
    auth.sslMode = QStringLiteral("prefer");
    auth.connectTimeout = 10;
    sm->saveDatabaseSettings(auth);
    sm->sync();
}

void DbSettingsDialog::setResult(const QString &text, bool success)
{
    ui->resultLabel->setText(text);
    if (success && !text.isEmpty()) {
        ui->resultLabel->setStyleSheet(QStringLiteral("QLabel { color: #080; font-weight: bold; }"));
    } else if (text.isEmpty()) {
        ui->resultLabel->setStyleSheet(QString());
    } else {
        ui->resultLabel->setStyleSheet(QStringLiteral("QLabel { color: #c00; }"));
    }
}

void DbSettingsDialog::on_testButton_clicked()
{
    PostgreSQLAuth auth;
    auth.host = ui->hostEdit->text().trimmed();
    if (auth.host.isEmpty()) auth.host = QStringLiteral("localhost");
    auth.port = ui->portEdit->text().trimmed().toInt();
    if (auth.port <= 0) auth.port = 5432;
    auth.database = ui->databaseEdit->text().trimmed();
    auth.username = ui->usernameEdit->text().trimmed();
    auth.password = ui->passwordEdit->text();
    auth.sslMode = QStringLiteral("prefer");
    auth.connectTimeout = 10;

    if (!auth.isValid()) {
        setResult(tr("Заполните хост, базу данных и пользователя."), false);
        return;
    }

    DatabaseConnection conn;
    if (conn.connect(auth)) {
        conn.disconnect();
        qInfo() << "DbSettingsDialog: test connection OK," << auth.host << auth.database;
        setResult(tr("Подключение успешно."), true);
    } else {
        qWarning() << "DbSettingsDialog: test connection failed," << conn.lastError().text();
        setResult(tr("Ошибка: %1").arg(conn.lastError().text()), false);
    }
}

void DbSettingsDialog::on_buttonBox_accepted()
{
    PostgreSQLAuth auth;
    auth.host = ui->hostEdit->text().trimmed();
    if (auth.host.isEmpty()) auth.host = QStringLiteral("localhost");
    auth.port = ui->portEdit->text().trimmed().toInt();
    if (auth.port <= 0) auth.port = 5432;
    auth.database = ui->databaseEdit->text().trimmed();
    auth.username = ui->usernameEdit->text().trimmed();
    auth.password = ui->passwordEdit->text();

    if (!auth.isValid()) {
        QMessageBox::warning(this, windowTitle(), tr("Заполните хост, базу данных и пользователя."));
        return;
    }

    saveToSettings();
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
        alerts->log(AlertCategory::System, AlertSignature::DbSettingsChanged,
                    tr("Настройки подключения к БД изменены: %1 / %2").arg(auth.host).arg(auth.database),
                    uid, empId);
    }
    qInfo() << "DbSettingsDialog: DB settings saved, host=" << auth.host << "db=" << auth.database;
    QMessageBox::information(this, windowTitle(),
        tr("Настройки сохранены.\nДля применения нового подключения перезапустите приложение."));
    accept();
}

void DbSettingsDialog::on_buttonBox_rejected()
{
    reject();
}
