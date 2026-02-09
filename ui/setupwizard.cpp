#include "setupwizard.h"
#include "setupdbpage.h"
#include "setupbrandingpage.h"
#include "setupadminpage.h"
#include "../easyposcore.h"
#include "../settings/settingsmanager.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include "../logging/logmanager.h"
#include "../db/databaseconnection.h"
#include "../RBAC/rolemanager.h"
#include "../RBAC/accountmanager.h"
#include "../RBAC/structures.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QWizardPage>
#include <QShowEvent>
#include <QMessageBox>
#include <QComboBox>
#include <QFormLayout>
#include <QSqlQuery>

SetupWizard::SetupWizard(std::shared_ptr<EasyPOSCore> core, QWidget *parent)
    : QWizard(parent)
    , m_core(core)
{
    setWindowTitle(tr("Первоначальная настройка %1").arg(m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS")));
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
    welcomePage->setSubTitle(tr("Мастер первоначальной настройки %1").arg(m_core ? m_core->getBrandingAppName() : QStringLiteral("easyPOS")));

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

    // --- Страница 3: Брендирование ---
    m_brandingPage = new SetupBrandingPage(this);
    m_brandingPage->setTitle(tr("Брендирование"));
    m_brandingPage->setSubTitle(tr("Название приложения, логотип, адрес и реквизиты для чеков"));
    addPage(m_brandingPage);

    // --- Страница 4: Первый пользователь (Администратор) ---
    m_adminPage = new SetupAdminPage(this);
    m_adminPage->setTitle(tr("Первый пользователь"));
    m_adminPage->setSubTitle(tr("Создайте учётную запись администратора с полным доступом"));
    m_adminPageId = addPage(m_adminPage);

    // --- Страница 5: Завершение ---
    QWizardPage *finishPage = new QWizardPage(this);
    finishPage->setTitle(tr("Настройка завершена"));
    finishPage->setSubTitle(tr("Параметры будут сохранены и приложение будет готово к работе."));

    QLabel *finishLabel = new QLabel(
        tr("Нажмите «Готово» для сохранения настроек и запуска приложения."));
    finishLabel->setWordWrap(true);

    QVBoxLayout *finishLayout = new QVBoxLayout(finishPage);
    finishLayout->addWidget(finishLabel);

    m_finishPageId = addPage(finishPage);

    connect(this, &QWizard::currentIdChanged, this, &SetupWizard::onCurrentIdChanged);
}

int SetupWizard::nextId() const
{
    const int id = currentId();
    if (id == m_dbPageId && m_dbPage) {
        PostgreSQLAuth auth = m_dbPage->getAuth();
        if (auth.isValid() && hasAdminRoleAndUser(auth)) {
            const_cast<SetupWizard *>(this)->m_skipBrandingAndAdmin = true;
            return m_finishPageId;
        }
    }
    return QWizard::nextId();
}

bool SetupWizard::hasAdminRoleAndUser(const PostgreSQLAuth &auth) const
{
    DatabaseConnection conn;
    if (!conn.connect(auth))
        return false;
    QSqlQuery q(conn.getDatabase());
    // Роль с level = 0 (администратор) и хотя бы один неудалённый пользователь с этой ролью
    const bool ok = q.exec(QLatin1String(
        "SELECT 1 FROM users u INNER JOIN roles r ON r.id = u.roleid "
        "WHERE r.level = 0 AND (r.isdeleted IS NULL OR r.isdeleted = false) "
        "AND (u.isdeleted IS NULL OR u.isdeleted = false) LIMIT 1"));
    conn.disconnect();
    return ok && q.next();
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
    m_core->getSettingsManager()->sync();
}

bool SetupWizard::saveBrandingAndAdmin()
{
    if (!m_core || !m_core->getDatabaseConnection() || !m_core->getDatabaseConnection()->isConnected())
        return false;

    // Брендирование
    if (m_brandingPage) {
        if (!m_core->saveBranding(m_brandingPage->appName(), m_brandingPage->logoPath(),
                                  m_brandingPage->address(), m_brandingPage->legalInfo())) {
            qWarning() << "SetupWizard: failed to save branding";
        }
    }

    // Роль «Администратор» (level 0)
    RoleManager *roleManager = m_core->createRoleManager(this);
    const QString adminRoleName = tr("Администратор");
    if (!roleManager->roleExists(adminRoleName)) {
        RoleOperationResult roleRes = roleManager->createRole(adminRoleName, AccessLevel::Admin);
        if (!roleRes.success) {
            qWarning() << "SetupWizard: failed to create role" << adminRoleName << roleRes.message;
        }
    }

    // Первый пользователь
    if (!m_adminPage)
        return true;
    AccountManager *accountManager = m_core->createAccountManager(this);
    UserOperationResult userRes = accountManager->registerUser(
                m_adminPage->username(), m_adminPage->password(), adminRoleName);
    if (!userRes.success) {
        QMessageBox::warning(this, tr("Ошибка создания пользователя"),
            tr("Не удалось создать учётную запись администратора:\n%1").arg(userRes.message));
        return false;
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        alerts->log(AlertCategory::Reference, AlertSignature::UserCreated,
                    tr("Первоначальная настройка: создан администратор %1").arg(m_adminPage->username()),
                    userRes.userId, 0);
    }
    return true;
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
    m_core->ensureDbConnection();
    if (!m_core->getDatabaseConnection() || !m_core->getDatabaseConnection()->isConnected()) {
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Не удалось подключиться к базе данных после сохранения настроек."));
        return;
    }
    if (!m_skipBrandingAndAdmin && !saveBrandingAndAdmin()) {
        if (m_adminPageId >= 0)
            setCurrentId(m_adminPageId);
        return;
    }
    m_core->getSettingsManager()->setValue(SettingsKeys::SetupCompleted, true);
    m_core->getSettingsManager()->sync();
    if (auto *alerts = m_core->createAlertsManager()) {
        alerts->log(AlertCategory::System, AlertSignature::DbSettingsChanged,
                    tr("Первоначальная настройка: БД %1 на %2").arg(auth.database).arg(auth.host), 0, 0);
    }
    qInfo() << "SetupWizard: setup completed, DB" << auth.host << auth.database;
    QWizard::accept();
}
