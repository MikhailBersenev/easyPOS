#include "setupwizard.h"
#include "setupdbpage.h"
#include "../easyposcore.h"
#include "../settings/settingsmanager.h"
#include "../logging/logmanager.h"
#include "../db/databaseconnection.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QWizardPage>
#include <QShowEvent>
#include <QMessageBox>
#include <QComboBox>
#include <QFormLayout>

SetupWizard::SetupWizard(std::shared_ptr<EasyPOSCore> core, QWidget *parent)
    : QWizard(parent)
    , m_core(core)
{
    setWindowTitle(tr("Первоначальная настройка easyPOS"));
    setFixedSize(550, 450);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoCancelButtonOnLastPage, false);

    setupPages();
}

void SetupWizard::showEvent(QShowEvent *event)
{
    QWizard::showEvent(event);
    setWindowState((windowState() & ~Qt::WindowMaximized) | Qt::WindowActive);
}

SetupWizard::~SetupWizard()
{
}

bool SetupWizard::setupRequired(std::shared_ptr<EasyPOSCore> core)
{
    if (!core || !core->getSettingsManager())
        return true;
    return !core->getSettingsManager()->boolValue(SettingsKeys::SetupCompleted, false);
}

void SetupWizard::setupPages()
{
    // --- Страница 1: Язык и приветствие ---
    QWizardPage *welcomePage = new QWizardPage(this);
    welcomePage->setTitle(tr("Добро пожаловать"));
    welcomePage->setSubTitle(tr("Мастер первоначальной настройки easyPOS"));

    m_languageCombo = new QComboBox(welcomePage);
    m_languageCombo->addItem(tr("По умолчанию (система)"), QStringLiteral(""));
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral("Русский"), QStringLiteral("ru"));
    if (m_core && m_core->getSettingsManager()) {
        QString saved = m_core->getSettingsManager()->stringValue(SettingsKeys::Language, QString());
        int idx = m_languageCombo->findData(saved);
        if (idx >= 0) m_languageCombo->setCurrentIndex(idx);
    }

    QLabel *welcomeLabel = new QLabel(
        tr("Для начала работы необходимо настроить подключение к базе данных PostgreSQL.\n\n"
           "Убедитесь, что сервер PostgreSQL запущен и у вас есть данные для подключения:\n"
           "хост, порт, имя базы данных, логин и пароль.\n\n"
           "Нажмите «Далее» для продолжения."));
    welcomeLabel->setWordWrap(true);
    welcomeLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomePage);
    QFormLayout *langRow = new QFormLayout();
    langRow->addRow(tr("Язык интерфейса:"), m_languageCombo);
    welcomeLayout->addLayout(langRow);
    welcomeLayout->addWidget(welcomeLabel);

    addPage(welcomePage);

    // --- Страница 2: Настройки БД (из .ui) ---
    m_dbPage = new SetupDbPage(this);
    m_dbPage->setTitle(tr("Подключение к базе данных"));
    m_dbPage->setSubTitle(tr("Введите параметры подключения к PostgreSQL"));
    m_dbPage->loadFromSettings(m_core ? m_core->getSettingsManager() : nullptr);

    connect(m_dbPage, &SetupDbPage::testConnectionRequested,
            this, &SetupWizard::onTestConnectionClicked);

    m_dbPageId = addPage(m_dbPage);

    // --- Страница 3: Завершение ---
    QWizardPage *finishPage = new QWizardPage(this);
    finishPage->setTitle(tr("Настройка завершена"));
    finishPage->setSubTitle(tr("Параметры будут сохранены и приложение будет готово к работе."));

    QLabel *finishLabel = new QLabel(
        tr("Нажмите «Готово» для сохранения настроек и запуска приложения."));
    finishLabel->setWordWrap(true);

    QVBoxLayout *finishLayout = new QVBoxLayout(finishPage);
    finishLayout->addWidget(finishLabel);

    addPage(finishPage);

    connect(this, &QWizard::currentIdChanged, this, &SetupWizard::onCurrentIdChanged);
}

void SetupWizard::onTestConnectionClicked()
{
    m_dbPage->setResult(tr("Проверка подключения..."), false);
    m_dbPage->setResultStyle(QStringLiteral("QLabel { color: gray; font-style: italic; }"));

    QApplication::processEvents();

    PostgreSQLAuth auth = m_dbPage->getAuth();
    if (!auth.isValid()) {
        m_dbPage->setResult(tr("Ошибка: заполните хост, базу данных и пользователя."), false);
        return;
    }

    DatabaseConnection conn;
    if (conn.connect(auth)) {
        m_dbPage->setResult(tr("Подключение успешно!"), true);
        conn.disconnect();
    } else {
        m_dbPage->setResult(tr("Ошибка подключения: %1").arg(conn.lastError().text()), false);
    }
}

void SetupWizard::onCurrentIdChanged(int id)
{
    Q_UNUSED(id);
}

void SetupWizard::saveSettings()
{
    if (!m_core || !m_core->getSettingsManager() || !m_dbPage)
        return;

    if (m_languageCombo) {
        QString lang = m_languageCombo->currentData().toString();
        m_core->getSettingsManager()->setValue(SettingsKeys::Language, lang);
    }
    PostgreSQLAuth auth = m_dbPage->getAuth();
    m_core->getSettingsManager()->saveDatabaseSettings(auth);
    m_core->getSettingsManager()->setValue(SettingsKeys::SetupCompleted, true);
    m_core->getSettingsManager()->sync();
}

void SetupWizard::accept()
{
    // Проверяем подключение перед завершением
    PostgreSQLAuth auth = m_dbPage->getAuth();
    if (!auth.isValid()) {
        if (m_dbPageId >= 0)
            setCurrentId(m_dbPageId);
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Заполните хост, базу данных и пользователя, затем проверьте подключение."));
        return;
    }

    DatabaseConnection conn;
    if (!conn.connect(auth)) {
        if (m_dbPageId >= 0)
            setCurrentId(m_dbPageId);
        QMessageBox::warning(this, tr("Ошибка подключения"),
            tr("Не удалось подключиться к базе данных:\n%1\n\nПроверьте параметры и нажмите «Проверить подключение».")
                .arg(conn.lastError().text()));
        return;
    }
    conn.disconnect();

    saveSettings();
    qInfo() << "SetupWizard: setup completed, DB" << auth.host << auth.database;
    QWizard::accept();
}
