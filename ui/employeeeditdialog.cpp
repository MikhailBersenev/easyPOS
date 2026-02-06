#include "employeeeditdialog.h"
#include "ui_employeeeditdialog.h"
#include "../easyposcore.h"
#include "../employees/structures.h"
#include "../employees/positionmanager.h"
#include "../RBAC/accountmanager.h"
#include "../RBAC/structures.h"
#include <QMessageBox>

EmployeeEditDialog::EmployeeEditDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : QDialog(parent)
    , ui(new Ui::EmployeeEditDialog)
    , m_core(core)
{
    ui->setupUi(this);
    fillPositionCombo();
    fillUserCombo();
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        if (lastName().isEmpty() || firstName().isEmpty()) {
            QMessageBox::warning(this, windowTitle(), tr("Введите фамилию и имя."));
            return;
        }
        if (positionId() <= 0) {
            QMessageBox::warning(this, windowTitle(), tr("Выберите должность."));
            return;
        }
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

EmployeeEditDialog::~EmployeeEditDialog()
{
    delete ui;
}

void EmployeeEditDialog::fillPositionCombo()
{
    ui->positionCombo->clear();
    if (!m_core || !m_core->getDatabaseConnection())
        return;
    PositionManager pm(this);
    pm.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<Position> list = pm.list(false);
    for (const Position &p : list)
        ui->positionCombo->addItem(p.name, p.id);
}

void EmployeeEditDialog::fillUserCombo()
{
    ui->userCombo->clear();
    ui->userCombo->addItem(tr("— Не привязан —"), 0);
    if (!m_core || !m_core->getDatabaseConnection())
        return;
    AccountManager *am = m_core->createAccountManager(this);
    if (!am)
        return;
    const QList<User> users = am->getAllUsers(false);
    for (const User &u : users)
        ui->userCombo->addItem(u.name, u.id);
}

void EmployeeEditDialog::setData(const Employee &emp)
{
    ui->lastNameEdit->setText(emp.lastName);
    ui->firstNameEdit->setText(emp.firstName);
    ui->middleNameEdit->setText(emp.middleName);
    const int posIdx = ui->positionCombo->findData(emp.position.id);
    ui->positionCombo->setCurrentIndex(posIdx >= 0 ? posIdx : 0);
    const int userIdx = ui->userCombo->findData(emp.userId);
    ui->userCombo->setCurrentIndex(userIdx >= 0 ? userIdx : 0);
    ui->badgeEdit->setText(emp.badgeCode);
}

QString EmployeeEditDialog::lastName() const
{
    return ui->lastNameEdit->text().trimmed();
}

QString EmployeeEditDialog::firstName() const
{
    return ui->firstNameEdit->text().trimmed();
}

QString EmployeeEditDialog::middleName() const
{
    return ui->middleNameEdit->text().trimmed();
}

int EmployeeEditDialog::positionId() const
{
    return ui->positionCombo->currentData().toInt();
}

int EmployeeEditDialog::userId() const
{
    return ui->userCombo->currentData().toInt();
}

QString EmployeeEditDialog::badgeCode() const
{
    return ui->badgeEdit->text().trimmed();
}
