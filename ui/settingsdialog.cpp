#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "dbsettingsdialog.h"
#include "../easyposcore.h"
#include "../settings/settingsmanager.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"
#include "../logging/logmanager.h"
#include "../RBAC/accountmanager.h"
#include "../RBAC/rolemanager.h"
#include "../RBAC/structures.h"
#include <QMessageBox>
#include <QPushButton>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimeZone>
#include <QSqlQuery>

SettingsDialog::SettingsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_core(core)
{
    ui->setupUi(this);
    if (QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel))
        cancelBtn->setObjectName("cancelButton");
    if (QVBoxLayout *mainLay = qobject_cast<QVBoxLayout *>(this->layout()))
        mainLay->setAlignment(ui->buttonBox, Qt::AlignRight);
    if (m_core)
        setWindowTitle(tr("Настройки — %1").arg(m_core->getBrandingAppName()));
    // Qt не задаёт userData элементов комбобокса из .ui — задаём вручную, иначе смена языка не сохраняется
    if (ui->languageComboBox->count() >= 3) {
        ui->languageComboBox->setItemData(0, QStringLiteral(""));
        ui->languageComboBox->setItemData(1, QStringLiteral("en"));
        ui->languageComboBox->setItemData(2, QStringLiteral("ru"));
    }
    // Заполняем список часовых поясов (IANA ID), первый пункт — системный
    ui->timeZoneComboBox->clear();
    ui->timeZoneComboBox->addItem(tr("По умолчанию (система)"), QStringLiteral(""));
    QList<QByteArray> tzIds = QTimeZone::availableTimeZoneIds();
    QStringList tzStrings;
    for (const QByteArray &id : tzIds)
        tzStrings.append(QString::fromUtf8(id));
    tzStrings.sort(Qt::CaseInsensitive);
    for (const QString &id : tzStrings)
        ui->timeZoneComboBox->addItem(id, id);
    if (m_core) {
        m_accountManager = m_core->createAccountManager(this);
        m_roleManager = m_core->createRoleManager(this);
    }
    loadFromSettings();
    loadRolesTable();
    loadUsersTable();
    ui->rolesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->rolesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->rolesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->usersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->usersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(ui->dbSettingsButton, &QPushButton::clicked, this, &SettingsDialog::onDbSettingsClicked);
    connect(ui->brandingLogoBrowseButton, &QPushButton::clicked, this, &SettingsDialog::onBrandingLogoBrowseClicked);
    ui->logPathBrowseButton->setAutoDefault(false);
    ui->logPathBrowseButton->setDefault(false);
    connect(ui->rolesAddBtn, &QPushButton::clicked, this, &SettingsDialog::onRolesAddClicked);
    connect(ui->rolesEditBtn, &QPushButton::clicked, this, &SettingsDialog::onRolesEditClicked);
    connect(ui->rolesDeleteBtn, &QPushButton::clicked, this, &SettingsDialog::onRolesDeleteClicked);
    connect(ui->usersAddBtn, &QPushButton::clicked, this, &SettingsDialog::onUsersAddClicked);
    connect(ui->usersEditBtn, &QPushButton::clicked, this, &SettingsDialog::onUsersEditClicked);
    connect(ui->usersDeleteBtn, &QPushButton::clicked, this, &SettingsDialog::onUsersDeleteClicked);
    connect(ui->usersRestoreBtn, &QPushButton::clicked, this, &SettingsDialog::onUsersRestoreClicked);
    connect(ui->showDeletedUsersCheckBox, &QCheckBox::toggled, this, [this]() { loadUsersTable(); });
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadFromSettings()
{
    if (!m_core || !m_core->getSettingsManager())
        return;
    auto *sm = m_core->getSettingsManager();
    QString lang = sm->stringValue(SettingsKeys::Language, QString());
    const int idx = ui->languageComboBox->findData(lang);
    ui->languageComboBox->setCurrentIndex(idx >= 0 ? idx : 0);
    QString tz = sm->stringValue(SettingsKeys::TimeZone, QString());
    const int tzIdx = ui->timeZoneComboBox->findData(tz);
    ui->timeZoneComboBox->setCurrentIndex(tzIdx >= 0 ? tzIdx : 0);
    ui->printAfterPayCheckBox->setChecked(sm->boolValue(SettingsKeys::PrintAfterPay, true));
    ui->logPathEdit->setText(sm->stringValue(SettingsKeys::LogPath, QString()));
    ui->brandingAppNameEdit->setText(m_core->getBrandingAppName());
    ui->brandingLogoPathEdit->setText(m_core->getBrandingLogoPath());
    ui->brandingAddressEdit->setText(m_core->getBrandingAddress());
    ui->brandingLegalEdit->setPlainText(m_core->getBrandingLegalInfo());

    // Ставка НДС по умолчанию
    ui->defaultVatRateComboBox->clear();
    ui->defaultVatRateComboBox->addItem(tr("По умолчанию (первая в списке)"), 0);
    if (m_core && m_core->getDatabaseConnection() && m_core->getDatabaseConnection()->isConnected()) {
        QSqlQuery q(m_core->getDatabaseConnection()->getDatabase());
        if (q.exec(QStringLiteral("SELECT id, name FROM vatrates WHERE (isdeleted IS NULL OR isdeleted = false) ORDER BY name"))) {
            while (q.next())
                ui->defaultVatRateComboBox->addItem(q.value(1).toString(), q.value(0).toLongLong());
        }
    }
    const qint64 savedVatId = sm->intValue(SettingsKeys::DefaultVatRateId, 0);
    const int vatIdx = ui->defaultVatRateComboBox->findData(savedVatId);
    ui->defaultVatRateComboBox->setCurrentIndex(vatIdx >= 0 ? vatIdx : 0);

    updateDbLabel();
}

void SettingsDialog::updateDbLabel()
{
    QString text = tr("Текущее подключение: ");
    if (m_core && m_core->getSettingsManager()) {
        QString host = m_core->getSettingsManager()->stringValue(SettingsKeys::DbHost, QStringLiteral("localhost"));
        QString db = m_core->getSettingsManager()->stringValue(SettingsKeys::DbName, QStringLiteral("pos_bakery"));
        if (host.isEmpty()) host = QStringLiteral("localhost");
        text += QStringLiteral("%1 / %2").arg(host, db);
    } else {
        text += tr("—");
    }
    ui->dbCurrentLabel->setText(text);
}

void SettingsDialog::onDbSettingsClicked()
{
    if (!m_core) return;
    DbSettingsDialog dlg(this, m_core);
    if (dlg.exec() == QDialog::Accepted)
        updateDbLabel();
}

void SettingsDialog::on_logPathBrowseButton_clicked()
{
    QString current = ui->logPathEdit->text().trimmed();
    if (!QDir(current).exists())
        current = QDir::homePath();

    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Выбрать каталог логов"),
        current,
        QFileDialog::DontUseNativeDialog
        );

    if (!dir.isEmpty()) {
        ui->logPathEdit->setText(QDir::toNativeSeparators(dir));
        ui->logPathEdit->setFocus();
    }
}

void SettingsDialog::onBrandingLogoBrowseClicked()
{
    QString current = ui->brandingLogoPathEdit->text().trimmed();
    QString dir = current.isEmpty() ? QDir::homePath() : QFileInfo(current).absolutePath();
    QString path = QFileDialog::getOpenFileName(this, tr("Выбрать логотип"), dir,
        tr("Изображения (*.png *.jpg *.jpeg *.bmp);;Все файлы (*)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (!path.isEmpty()) {
        ui->brandingLogoPathEdit->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsDialog::loadRolesTable()
{
    ui->rolesTable->setRowCount(0);
    if (!m_roleManager || !m_core->getDatabaseConnection() || !m_core->getDatabaseConnection()->isConnected())
        return;
    const QList<Role> roles = m_roleManager->getAllRoles(false);
    for (const Role &r : roles) {
        const int row = ui->rolesTable->rowCount();
        ui->rolesTable->insertRow(row);
        ui->rolesTable->setItem(row, 0, new QTableWidgetItem(r.name));
        ui->rolesTable->setItem(row, 1, new QTableWidgetItem(QString::number(r.level)));
        ui->rolesTable->setItem(row, 2, new QTableWidgetItem(r.isBlocked ? tr("Да") : tr("Нет")));
        ui->rolesTable->item(row, 0)->setData(Qt::UserRole, r.id);
    }
}

void SettingsDialog::loadUsersTable()
{
    ui->usersTable->setRowCount(0);
    if (!m_accountManager || !m_core->getDatabaseConnection() || !m_core->getDatabaseConnection()->isConnected())
        return;
    const bool includeDeleted = ui->showDeletedUsersCheckBox->isChecked();
    const QList<User> users = m_accountManager->getAllUsers(includeDeleted);
    for (const User &u : users) {
        const int row = ui->usersTable->rowCount();
        ui->usersTable->insertRow(row);
        QString nameDisplay = u.name;
        if (u.isDeleted) nameDisplay += tr(" (удалён)");
        ui->usersTable->setItem(row, 0, new QTableWidgetItem(nameDisplay));
        ui->usersTable->setItem(row, 1, new QTableWidgetItem(u.role.name));
        ui->usersTable->item(row, 0)->setData(Qt::UserRole, u.id);
        ui->usersTable->item(row, 0)->setData(Qt::UserRole + 1, u.isDeleted);
    }
}

void SettingsDialog::onRolesAddClicked()
{
    if (!m_roleManager) return;
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Новая роль"));
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(tr("Название роли"));
    auto *levelSpin = new QSpinBox(&dlg);
    levelSpin->setRange(0, 999);
    levelSpin->setValue(0);
    auto *form = new QFormLayout(&dlg);
    form->addRow(tr("Название:"), nameEdit);
    form->addRow(tr("Уровень (0 — высший):"), levelSpin);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);
    if (dlg.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) return;
    RoleOperationResult r = m_roleManager->createRole(nameEdit->text().trimmed(), levelSpin->value());
    if (r.success) {
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::RoleCreated,
                       tr("Роль создана: %1, уровень %2").arg(nameEdit->text().trimmed()).arg(levelSpin->value()),
                       uid, empId);
        }
        QMessageBox::information(this, tr("Роли"), r.message);
        loadRolesTable();
    } else {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
    }
}

void SettingsDialog::onRolesEditClicked()
{
    if (!m_roleManager) return;
    const int row = ui->rolesTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Роли"), tr("Выберите роль для редактирования."));
        return;
    }
    QTableWidgetItem *first = ui->rolesTable->item(row, 0);
    if (!first) return;
    const int roleId = first->data(Qt::UserRole).toInt();
    Role role = m_roleManager->getRole(roleId, true);
    if (role.id == 0) return;
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Изменить роль"));
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setText(role.name);
    auto *levelSpin = new QSpinBox(&dlg);
    levelSpin->setRange(0, 999);
    levelSpin->setValue(role.level);
    auto *blockedCheck = new QCheckBox(tr("Заблокирована"), &dlg);
    blockedCheck->setChecked(role.isBlocked);
    auto *form = new QFormLayout(&dlg);
    form->addRow(tr("Название:"), nameEdit);
    form->addRow(tr("Уровень:"), levelSpin);
    form->addRow(blockedCheck);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);
    if (dlg.exec() != QDialog::Accepted || nameEdit->text().trimmed().isEmpty()) return;
    RoleOperationResult r = m_roleManager->updateRole(roleId, nameEdit->text().trimmed(), levelSpin->value());
    if (r.success) {
        if (blockedCheck->isChecked() != role.isBlocked) {
            m_roleManager->blockRole(roleId, blockedCheck->isChecked());
            if (auto *alerts = m_core->createAlertsManager()) {
                qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
                qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
                alerts->log(AlertCategory::Reference, AlertSignature::RoleBlocked,
                            blockedCheck->isChecked() ? tr("Роль «%1» заблокирована").arg(nameEdit->text().trimmed())
                                                       : tr("Роль «%1» разблокирована").arg(nameEdit->text().trimmed()),
                            uid, empId);
            }
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::RoleUpdated,
                       tr("Роль обновлена: id=%1, %2").arg(roleId).arg(nameEdit->text().trimmed()),
                       uid, empId);
        }
        QMessageBox::information(this, tr("Роли"), tr("Роль обновлена."));
        loadRolesTable();
    } else {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
    }
}

void SettingsDialog::onRolesDeleteClicked()
{
    if (!m_roleManager) return;
    const int row = ui->rolesTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Роли"), tr("Выберите роль для удаления."));
        return;
    }
    QTableWidgetItem *first = ui->rolesTable->item(row, 0);
    if (!first) return;
    const int roleId = first->data(Qt::UserRole).toInt();
    QString name = first->text();
    if (QMessageBox::question(this, tr("Удаление роли"),
            tr("Пометить роль «%1» как удалённую?").arg(name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    RoleOperationResult r = m_roleManager->deleteRole(roleId);
    if (r.success) {
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::RoleDeleted,
                       tr("Роль удалена: «%1», id=%2").arg(name).arg(roleId), uid, empId);
        }
        QMessageBox::information(this, tr("Роли"), r.message);
        loadRolesTable();
    } else {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
    }
}

void SettingsDialog::onUsersAddClicked()
{
    if (!m_accountManager || !m_roleManager) return;
    const QList<Role> roles = m_roleManager->getAllRoles(false);
    if (roles.isEmpty()) {
        QMessageBox::warning(this, tr("Пользователи"), tr("Сначала создайте хотя бы одну роль."));
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Новый пользователь"));
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(tr("Имя входа"));
    auto *passwordEdit = new QLineEdit(&dlg);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Пароль"));
    auto *roleCombo = new QComboBox(&dlg);
    for (const Role &r : roles)
        roleCombo->addItem(r.name, r.id);
    auto *form = new QFormLayout(&dlg);
    form->addRow(tr("Имя пользователя:"), nameEdit);
    form->addRow(tr("Пароль:"), passwordEdit);
    form->addRow(tr("Роль:"), roleCombo);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);
    if (dlg.exec() != QDialog::Accepted) return;
    const QString username = nameEdit->text().trimmed();
    const QString password = passwordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Пользователи"), tr("Укажите имя и пароль."));
        return;
    }
    const int roleId = roleCombo->currentData().toInt();
    UserOperationResult r = m_accountManager->registerUser(username, password, roleId);
    if (r.success) {
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::UserCreated,
                       tr("Пользователь создан: %1, рольId=%2").arg(username).arg(roleId), uid, empId);
        }
        QMessageBox::information(this, tr("Пользователи"), r.message);
        loadUsersTable();
    } else {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
    }
}

void SettingsDialog::onUsersEditClicked()
{
    if (!m_accountManager || !m_roleManager) return;
    const int row = ui->usersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Пользователи"), tr("Выберите пользователя для редактирования."));
        return;
    }
    QTableWidgetItem *first = ui->usersTable->item(row, 0);
    if (!first) return;
    const int userId = first->data(Qt::UserRole).toInt();
    if (first->data(Qt::UserRole + 1).toBool()) {
        QMessageBox::information(this, tr("Пользователи"), tr("Сначала восстановите удалённого пользователя."));
        return;
    }
    User u = m_accountManager->getUser(userId, false);
    if (u.id == 0) return;
    const QList<Role> roles = m_roleManager->getAllRoles(false);
    if (roles.isEmpty()) return;
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Изменить пользователя"));
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setReadOnly(true);
    nameEdit->setText(u.name);
    auto *roleCombo = new QComboBox(&dlg);
    int roleIndex = 0;
    for (int i = 0; i < roles.size(); ++i) {
        roleCombo->addItem(roles[i].name, roles[i].id);
        if (roles[i].id == u.role.id) roleIndex = i;
    }
    roleCombo->setCurrentIndex(roleIndex);
    auto *passwordEdit = new QLineEdit(&dlg);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("Оставить без изменений"));
    auto *form = new QFormLayout(&dlg);
    form->addRow(tr("Имя пользователя:"), nameEdit);
    form->addRow(tr("Роль:"), roleCombo);
    form->addRow(tr("Новый пароль:"), passwordEdit);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);
    if (dlg.exec() != QDialog::Accepted) return;
    UserOperationResult r = m_accountManager->updateUserRole(userId, roleCombo->currentData().toInt());
    if (!r.success) {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
        return;
    }
    if (!passwordEdit->text().isEmpty()) {
        r = m_accountManager->updateUserPassword(userId, passwordEdit->text());
        if (!r.success) {
            QMessageBox::warning(this, tr("Ошибка"), r.message);
            return;
        }
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
        alerts->log(AlertCategory::Reference, AlertSignature::UserUpdated,
                    tr("Пользователь обновлён: %1 (роль, пароль)").arg(u.name), uid, empId);
    }
    QMessageBox::information(this, tr("Пользователи"), tr("Данные пользователя обновлены."));
    loadUsersTable();
}

void SettingsDialog::onUsersDeleteClicked()
{
    if (!m_accountManager) return;
    const int row = ui->usersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Пользователи"), tr("Выберите пользователя для удаления."));
        return;
    }
    QTableWidgetItem *first = ui->usersTable->item(row, 0);
    if (!first) return;
    if (first->data(Qt::UserRole + 1).toBool()) {
        QMessageBox::information(this, tr("Пользователи"), tr("Пользователь уже удалён. Используйте «Восстановить»."));
        return;
    }
    const int userId = first->data(Qt::UserRole).toInt();
    QString name = first->text();
    if (name.endsWith(tr(" (удалён)"))) name.chop(tr(" (удалён)").length());
    if (QMessageBox::question(this, tr("Удаление пользователя"),
            tr("Пометить пользователя «%1» как удалённого?").arg(name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    UserOperationResult r = m_accountManager->deleteUser(userId);
    if (r.success) {
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::UserDeleted,
                       tr("Пользователь удалён: «%1», id=%2").arg(name).arg(userId), uid, empId);
        }
        QMessageBox::information(this, tr("Пользователи"), r.message);
        loadUsersTable();
    } else {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
    }
}

void SettingsDialog::onUsersRestoreClicked()
{
    if (!m_accountManager) return;
    const int row = ui->usersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Пользователи"), tr("Выберите удалённого пользователя для восстановления."));
        return;
    }
    QTableWidgetItem *first = ui->usersTable->item(row, 0);
    if (!first) return;
    if (!first->data(Qt::UserRole + 1).toBool()) {
        QMessageBox::information(this, tr("Пользователи"), tr("Включите «Показать удалённых» и выберите пользователя с пометкой «(удалён)»."));
        return;
    }
    const int userId = first->data(Qt::UserRole).toInt();
    UserOperationResult r = m_accountManager->restoreUser(userId);
    if (r.success) {
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::UserRestored,
                       tr("Пользователь восстановлен: id=%1").arg(userId), uid, empId);
        }
        QMessageBox::information(this, tr("Пользователи"), r.message);
        loadUsersTable();
    } else {
        QMessageBox::warning(this, tr("Ошибка"), r.message);
    }
}

void SettingsDialog::accept()
{
    if (m_core && m_core->getSettingsManager()) {
        auto *sm = m_core->getSettingsManager();
        QString lang = ui->languageComboBox->currentData().toString();
        QString logPath = ui->logPathEdit->text().trimmed();
        QString prevLogPath = sm->stringValue(SettingsKeys::LogPath, QString());
        if (prevLogPath != logPath) {
            qInfo() << "SettingsDialog: смена каталога логов:" << (prevLogPath.isEmpty() ? "по умолчанию" : prevLogPath)
                    << "->" << (logPath.isEmpty() ? "по умолчанию" : logPath)
                    << "(применится после перезапуска)";
        }
        sm->setValue(SettingsKeys::Language, lang);
        sm->setValue(SettingsKeys::TimeZone, ui->timeZoneComboBox->currentData().toString());
        sm->setValue(SettingsKeys::PrintAfterPay, ui->printAfterPayCheckBox->isChecked());
        const qint64 vatId = ui->defaultVatRateComboBox->currentData().toLongLong();
        sm->setValue(SettingsKeys::DefaultVatRateId, static_cast<int>(vatId));
        sm->setValue(SettingsKeys::LogPath, logPath);
        sm->sync();
        if (m_core->isDatabaseConnected()) {
            const QString appName = ui->brandingAppNameEdit->text().trimmed();
            const QString logoPath = ui->brandingLogoPathEdit->text().trimmed();
            const QString address = ui->brandingAddressEdit->text().trimmed();
            const QString legalInfo = ui->brandingLegalEdit->toPlainText().trimmed();
            if (m_core->saveBranding(appName.isEmpty() ? QStringLiteral("easyPOS") : appName, logoPath, address, legalInfo)) {
                if (auto *alerts = m_core->createAlertsManager()) {
                    qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
                    qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
                    alerts->log(AlertCategory::System, AlertSignature::SettingsChanged,
                        tr("Брендирование сохранено в БД"), uid, empId);
                }
            } else {
                QMessageBox::warning(this, tr("Настройки"),
                    tr("Не удалось сохранить брендирование в базу данных. Проверьте подключение к БД."));
            }
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::System, AlertSignature::SettingsChanged,
                        tr("Настройки сохранены (язык, часовой пояс, НДС, печать, логи)"), uid, empId);
        }
        qInfo() << "SettingsDialog: settings saved (language=" << lang << "logPath=" << (logPath.isEmpty() ? "default" : logPath) << ")";
    }
    QDialog::accept();
}
