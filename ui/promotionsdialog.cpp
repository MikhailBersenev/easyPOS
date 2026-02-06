#include "promotionsdialog.h"
#include "promotioneditdialog.h"
#include "../easyposcore.h"
#include "../promotions/promotionmanager.h"
#include "../promotions/structures.h"
#include "../alerts/alertkeys.h"
#include "../alerts/alertsmanager.h"

PromotionsDialog::PromotionsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Маркетинговые акции"))
{
    setupColumns({tr("ID"), tr("Название"), tr("Описание"), tr("Скидка %"), tr("Скидка ₽")},
                 {50, 180, 200, 80, 90});
    loadTable();
}

PromotionsDialog::~PromotionsDialog()
{
}

void PromotionsDialog::loadTable(bool includeDeleted)
{
    clearTable();
    if (!m_core || !m_core->getDatabaseConnection())
        return;

    PromotionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<Promotion> list = mgr.list(includeDeleted);

    for (const Promotion &p : list) {
        const int row = insertRow();
        setCellText(row, 0, QString::number(p.id));
        setCellText(row, 1, p.name);
        setCellText(row, 2, p.description);
        setCellText(row, 3, p.percent > 0 ? QString::number(p.percent, 'f', 1) : QString());
        setCellText(row, 4, p.sum > 0 ? QString::number(p.sum, 'f', 2) : QString());
    }
    updateRecordCount();
}

void PromotionsDialog::addRecord()
{
    addOrEditPromotion(nullptr);
}

void PromotionsDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;
    const int id = cellText(row, 0).toInt();
    PromotionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<Promotion> list = mgr.list(true);
    Promotion existing;
    for (const Promotion &p : list) {
        if (p.id == id) {
            existing = p;
            break;
        }
    }
    if (existing.id == 0)
        return;
    addOrEditPromotion(&existing);
}

void PromotionsDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;
    const int id = cellText(row, 0).toInt();
    const QString name = cellText(row, 1);
    if (!askConfirmation(tr("Пометить акцию «%1» как удалённую?").arg(name)))
        return;

    PromotionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    if (!mgr.setDeleted(id, true)) {
        showError(tr("Не удалось удалить: %1").arg(mgr.lastError().text()));
        return;
    }
    if (auto *alerts = m_core->createAlertsManager()) {
        qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
        qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
        alerts->log(AlertCategory::Reference, AlertSignature::PromotionDeleted,
                   tr("Акция помечена удалённой: %1 (id=%2)").arg(name).arg(id), uid, empId);
    }
    showInfo(tr("Акция помечена как удалённая."));
    loadTable();
}

void PromotionsDialog::addOrEditPromotion(const Promotion *existing)
{
    PromotionEditDialog dlg(this);
    if (existing) {
        dlg.setWindowTitle(tr("Редактирование акции"));
        dlg.setData(existing->name, existing->description, existing->percent, existing->sum);
    } else {
        dlg.setWindowTitle(tr("Новая акция"));
    }
    if (dlg.exec() != QDialog::Accepted)
        return;

    PromotionManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());

    if (existing) {
        Promotion promo = *existing;
        promo.name = dlg.name();
        promo.description = dlg.description();
        promo.percent = dlg.percent();
        promo.sum = dlg.sum();
        if (!mgr.update(promo)) {
            showError(tr("Не удалось сохранить: %1").arg(mgr.lastError().text()));
            return;
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::PromotionUpdated,
                       tr("Акция обновлена: %1 (id=%2)").arg(promo.name).arg(promo.id), uid, empId);
        }
        showInfo(tr("Акция обновлена."));
    } else {
        Promotion promo;
        promo.name = dlg.name();
        promo.description = dlg.description();
        promo.percent = dlg.percent();
        promo.sum = dlg.sum();
        if (!mgr.add(promo)) {
            showError(tr("Не удалось добавить: %1").arg(mgr.lastError().text()));
            return;
        }
        if (auto *alerts = m_core->createAlertsManager()) {
            qint64 uid = m_core->hasActiveSession() ? m_core->getCurrentSession().userId : 0;
            qint64 empId = uid > 0 ? m_core->getEmployeeIdByUserId(uid) : 0;
            alerts->log(AlertCategory::Reference, AlertSignature::PromotionCreated,
                       tr("Акция добавлена: %1 (id=%2)").arg(promo.name).arg(promo.id), uid, empId);
        }
        showInfo(tr("Акция добавлена."));
    }
    loadTable();
}
