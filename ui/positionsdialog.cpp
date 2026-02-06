#include "positionsdialog.h"
#include "positioneditdialog.h"
#include "../easyposcore.h"
#include "../employees/positionmanager.h"
#include "../employees/structures.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"

PositionsDialog::PositionsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Должности"))
{
    setupColumns({tr("ID"), tr("Название"), tr("Активна"), tr("Дата обновления")},
                 {50, 220, 70, 120});
    loadTable();
}

PositionsDialog::~PositionsDialog()
{
}

void PositionsDialog::loadTable(bool includeDeleted)
{
    clearTable();
    if (!m_core || !m_core->getDatabaseConnection())
        return;

    PositionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<Position> list = mgr.list(includeDeleted);

    for (const Position &p : list) {
        const int row = insertRow();
        setCellText(row, 0, QString::number(p.id));
        setCellText(row, 1, p.name);
        setCellText(row, 2, p.isActive ? tr("Да") : tr("Нет"));
        setCellText(row, 3, p.updateDate.toString(Qt::ISODate));
    }
    updateRecordCount();
}

void PositionsDialog::addRecord()
{
    addOrEditPosition(nullptr);
}

void PositionsDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;
    Position pos;
    pos.id = cellText(row, 0).toInt();
    pos.name = cellText(row, 1);
    pos.isActive = cellText(row, 2) == tr("Да");
    addOrEditPosition(&pos);
}

void PositionsDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;
    const int id = cellText(row, 0).toInt();
    const QString name = cellText(row, 1);
    if (!askConfirmation(tr("Пометить должность «%1» как удалённую?").arg(name)))
        return;

    PositionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    if (!mgr.setDeleted(id, true)) {
        showError(tr("Не удалось удалить: %1").arg(mgr.lastError().text()));
        return;
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
        alerts->log(AlertCategory::Reference, AlertSignature::PositionDeleted,
                   tr("Должность помечена удалённой: %1 (id=%2)").arg(name).arg(id), uid, empId);
    }
    showInfo(tr("Должность помечена как удалённая."));
    loadTable();
}

void PositionsDialog::addOrEditPosition(const Position *existing)
{
    PositionEditDialog dlg(this);
    if (existing) {
        dlg.setWindowTitle(tr("Редактирование должности"));
        dlg.setData(existing->name, existing->isActive);
    } else {
        dlg.setWindowTitle(tr("Новая должность"));
    }
    if (dlg.exec() != QDialog::Accepted)
        return;

    PositionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());

    if (existing) {
        Position pos = *existing;
        pos.name = dlg.name();
        pos.isActive = dlg.isActive();
        if (!mgr.update(pos)) {
            showError(tr("Не удалось сохранить: %1").arg(mgr.lastError().text()));
            return;
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::PositionUpdated,
                       tr("Должность обновлена: %1 (id=%2)").arg(pos.name).arg(pos.id), uid, empId);
        }
        showInfo(tr("Должность обновлена."));
    } else {
        Position pos;
        pos.name = dlg.name();
        pos.isActive = dlg.isActive();
        if (!mgr.add(pos)) {
            showError(tr("Не удалось добавить: %1").arg(mgr.lastError().text()));
            return;
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::PositionCreated,
                       tr("Должность добавлена: %1 (id=%2)").arg(pos.name).arg(pos.id), uid, empId);
        }
        showInfo(tr("Должность добавлена."));
    }
    loadTable();
}
