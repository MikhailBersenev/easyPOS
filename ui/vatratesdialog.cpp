#include "vatratesdialog.h"
#include "vatrateeditdialog.h"
#include "../easyposcore.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>

VatRatesDialog::VatRatesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Ставки НДС"))
{
    setupColumns({tr("ID"), tr("Название"), tr("Ставка %")}, {50, 200, 100});
    loadTable();
}

VatRatesDialog::~VatRatesDialog()
{
}

void VatRatesDialog::loadTable(bool includeDeleted)
{
    clearTable();
    if (!m_core || !m_core->getDatabaseConnection()) return;

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    QString sql = QStringLiteral("SELECT id, name, rate FROM vatrates");
    if (!includeDeleted)
        sql += QStringLiteral(" WHERE (isdeleted IS NULL OR isdeleted = false)");
    sql += QStringLiteral(" ORDER BY rate");
    if (!q.exec(sql)) {
        showError(tr("Ошибка загрузки: %1").arg(q.lastError().text()));
        return;
    }
    while (q.next()) {
        const int row = insertRow();
        setCellText(row, 0, q.value(0).toString());
        setCellText(row, 1, q.value(1).toString());
        setCellText(row, 2, QString::number(q.value(2).toDouble(), 'f', 2));
    }
    updateRecordCount();
}

void VatRatesDialog::addOrEditVatRate(int existingId)
{
    QString name;
    double rate = 0;
    if (existingId > 0) {
        const int row = currentRow();
        if (row < 0) return;
        name = cellText(row, 1);
        rate = cellText(row, 2).toDouble();
    } else {
        name = tr("НДС 20%");
        rate = 20;
    }

    VatRateEditDialog dlg(this);
    dlg.setData(name, rate);
    dlg.setWindowTitle(existingId > 0 ? tr("Редактирование ставки НДС") : tr("Новая ставка НДС"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    name = dlg.name();
    rate = dlg.rate();

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);

    if (existingId > 0) {
        q.prepare(QStringLiteral("UPDATE vatrates SET name = :name, rate = :rate WHERE id = :id"));
        q.bindValue(QStringLiteral(":name"), name.trimmed());
        q.bindValue(QStringLiteral(":rate"), rate);
        q.bindValue(QStringLiteral(":id"), existingId);
        if (!q.exec()) {
            showError(tr("Ошибка сохранения: %1").arg(q.lastError().text()));
            return;
        }
        showInfo(tr("Ставка НДС обновлена."));
    } else {
        q.prepare(QStringLiteral(
            "INSERT INTO vatrates (name, rate, isdeleted) VALUES (:name, :rate, false) RETURNING id"));
        q.bindValue(QStringLiteral(":name"), name.trimmed());
        q.bindValue(QStringLiteral(":rate"), rate);
        if (!q.exec() || !q.next()) {
            showError(tr("Ошибка добавления: %1").arg(q.lastError().text()));
            return;
        }
        showInfo(tr("Ставка НДС добавлена."));
    }
    loadTable();
}

void VatRatesDialog::addRecord()
{
    addOrEditVatRate(0);
}

void VatRatesDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0) {
        showWarning(tr("Выберите ставку для редактирования."));
        return;
    }
    addOrEditVatRate(cellText(row, 0).toInt());
}

void VatRatesDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0) return;
    const int id = cellText(row, 0).toInt();
    const QString name = cellText(row, 1);
    if (!askConfirmation(tr("Пометить ставку «%1» как удалённую?").arg(name))) return;

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE vatrates SET isdeleted = true WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        showError(tr("Ошибка: %1").arg(q.lastError().text()));
        return;
    }
    showInfo(tr("Ставка НДС помечена как удалённая."));
    loadTable();
}
