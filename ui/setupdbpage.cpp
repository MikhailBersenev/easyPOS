#include "setupdbpage.h"
#include "ui_setupdbpage.h"
#include "../settings/settingsmanager.h"

SetupDbPage::SetupDbPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::SetupDbPage)
{
    ui->setupUi(this);
}

SetupDbPage::~SetupDbPage()
{
    delete ui;
}

PostgreSQLAuth SetupDbPage::getAuth() const
{
    PostgreSQLAuth auth;
    auth.host = ui->hostEdit->text().trimmed();
    if (auth.host.isEmpty())
        auth.host = QStringLiteral("localhost");
    auth.port = ui->portEdit->text().trimmed().toInt();
    if (auth.port <= 0)
        auth.port = 5432;
    auth.database = ui->databaseEdit->text().trimmed();
    auth.username = ui->usernameEdit->text().trimmed();
    auth.password = ui->passwordEdit->text();
    auth.sslMode = QStringLiteral("prefer");
    auth.connectTimeout = 10;
    return auth;
}

void SetupDbPage::setResult(const QString &text, bool success)
{
    ui->resultLabel->setText(text);
    if (success) {
        ui->resultLabel->setStyleSheet(QStringLiteral("QLabel { color: #080; font-weight: bold; }"));
    } else if (text.isEmpty()) {
        ui->resultLabel->setStyleSheet(QStringLiteral("QLabel { color: gray; font-style: italic; }"));
    } else {
        ui->resultLabel->setStyleSheet(QStringLiteral("QLabel { color: #c00; }"));
    }
}

void SetupDbPage::setResultStyle(const QString &styleSheet)
{
    ui->resultLabel->setStyleSheet(styleSheet);
}

void SetupDbPage::loadFromSettings(SettingsManager *sm)
{
    if (!sm)
        return;
    ui->hostEdit->setText(sm->stringValue(SettingsKeys::DbHost, QStringLiteral("localhost")));
    ui->portEdit->setText(QString::number(sm->intValue(SettingsKeys::DbPort, 5432)));
    ui->databaseEdit->setText(sm->stringValue(SettingsKeys::DbName, QStringLiteral("pos_bakery")));
    ui->usernameEdit->setText(sm->stringValue(SettingsKeys::DbUsername, QStringLiteral("postgres")));
    if (sm->contains(SettingsKeys::DbPassword))
        ui->passwordEdit->setText(sm->stringValue(SettingsKeys::DbPassword));
}

void SetupDbPage::on_testButton_clicked()
{
    emit testConnectionRequested();
}
