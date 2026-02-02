#include "servicesdialog.h"
#include "serviceeditdialog.h"
#include "../easyposcore.h"
#include "../goods/categorymanager.h"
#include "../goods/structures.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

ServicesDialog::ServicesDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Услуги"))
{
    setupColumns({tr("ID"), tr("Название"), tr("Описание"), tr("Цена"), tr("Категория"), tr("Активен")},
                 {50, 180, 200, 80, 120, 70});
    loadTable();
}

ServicesDialog::~ServicesDialog()
{
}

void ServicesDialog::loadTable(bool includeDeleted)
{
    clearTable();
    if (!m_core || !m_core->getDatabaseConnection()) return;

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    QString sql = QStringLiteral(
        "SELECT s.id, s.name, s.description, s.price, c.name, s.isactive "
        "FROM services s LEFT JOIN goodcats c ON s.categoryid = c.id");
    if (!includeDeleted)
        sql += QStringLiteral(" WHERE (s.isdeleted IS NULL OR s.isdeleted = false)");
    sql += QStringLiteral(" ORDER BY s.name");
    if (!q.exec(sql)) {
        showError(tr("Ошибка загрузки: %1").arg(q.lastError().text()));
        return;
    }
    while (q.next()) {
        const int row = insertRow();
        setCellText(row, 0, q.value(0).toString());
        setCellText(row, 1, q.value(1).toString());
        setCellText(row, 2, q.value(2).toString());
        setCellText(row, 3, QString::number(q.value(3).toDouble(), 'f', 2));
        setCellText(row, 4, q.value(4).toString());
        setCellText(row, 5, q.value(5).toBool() ? tr("Да") : tr("Нет"));
    }
    updateRecordCount();
}

void ServicesDialog::addOrEditService(int existingId)
{
    QString name, description;
    double price = 0;
    int categoryId = 1;

    if (existingId > 0) {
        const int row = currentRow();
        if (row < 0) return;
        name = cellText(row, 1);
        description = cellText(row, 2);
        price = cellText(row, 3).toDouble();
        QSqlQuery sq(m_core->getDatabaseConnection()->getDatabase());
        sq.prepare(QStringLiteral("SELECT categoryid FROM services WHERE id = :id"));
        sq.bindValue(QStringLiteral(":id"), existingId);
        if (sq.exec() && sq.next())
            categoryId = sq.value(0).toInt();
    }

    CategoryManager catMgr(this);
    catMgr.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<GoodCategory> cats = catMgr.list();
    QList<int> catIds;
    QStringList catNames;
    for (const GoodCategory &c : cats) {
        catIds << c.id;
        catNames << c.name;
    }
    if (catNames.isEmpty()) {
        showError(tr("Нет категорий. Сначала добавьте категории товаров."));
        return;
    }

    ServiceEditDialog dlg(this);
    dlg.setCategories(catIds, catNames);
    dlg.setData(name, description, price, categoryId);
    dlg.setWindowTitle(existingId > 0 ? tr("Редактирование услуги") : tr("Новая услуга"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    name = dlg.name();
    description = dlg.description();
    price = dlg.price();
    categoryId = dlg.categoryId();

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);

    if (existingId > 0) {
        q.prepare(QStringLiteral(
            "UPDATE services SET name = :name, description = :desc, price = :price, "
            "categoryid = :catid, updatedate = :ud WHERE id = :id"));
        q.bindValue(QStringLiteral(":name"), name.trimmed());
        q.bindValue(QStringLiteral(":desc"), description.trimmed());
        q.bindValue(QStringLiteral(":price"), price);
        q.bindValue(QStringLiteral(":catid"), categoryId);
        q.bindValue(QStringLiteral(":ud"), QDate::currentDate());
        q.bindValue(QStringLiteral(":id"), existingId);
        if (!q.exec()) {
            showError(tr("Ошибка сохранения: %1").arg(q.lastError().text()));
            return;
        }
        showInfo(tr("Услуга обновлена."));
    } else {
        q.prepare(QStringLiteral(
            "INSERT INTO services (name, description, price, updatedate, isactive, categoryid, isdeleted) "
            "VALUES (:name, :desc, :price, :ud, true, :catid, false) RETURNING id"));
        q.bindValue(QStringLiteral(":name"), name.trimmed());
        q.bindValue(QStringLiteral(":desc"), description.trimmed());
        q.bindValue(QStringLiteral(":price"), price);
        q.bindValue(QStringLiteral(":ud"), QDate::currentDate());
        q.bindValue(QStringLiteral(":catid"), categoryId);
        if (!q.exec() || !q.next()) {
            showError(tr("Ошибка добавления: %1").arg(q.lastError().text()));
            return;
        }
        showInfo(tr("Услуга добавлена."));
    }
    loadTable();
}

void ServicesDialog::addRecord()
{
    addOrEditService(0);
}

void ServicesDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0) {
        showWarning(tr("Выберите услугу для редактирования."));
        return;
    }
    const int id = cellText(row, 0).toInt();
    addOrEditService(id);
}

void ServicesDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0) return;
    const int id = cellText(row, 0).toInt();
    const QString name = cellText(row, 1);
    if (!askConfirmation(tr("Пометить услугу «%1» как удалённую?").arg(name))) return;

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE services SET isdeleted = true WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        showError(tr("Ошибка: %1").arg(q.lastError().text()));
        return;
    }
    showInfo(tr("Услуга помечена как удалённая."));
    loadTable();
}
