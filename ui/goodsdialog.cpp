#include "goodsdialog.h"
#include "goodeditdialog.h"
#include "../easyposcore.h"
#include "../goods/categorymanager.h"
#include "../goods/structures.h"
#include "../db/databaseconnection.h"
#include "../RBAC/structures.h"
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>

GoodsDialog::GoodsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Товары"))
{
    // Настройка колонок: ID, Название, Описание, Категория, Активен
    setupColumns({tr("ID"), tr("Название"), tr("Описание"), tr("Категория"), tr("Активен")},
                 {50, 200, 250, 120, 80});
    
    loadTable();
}

GoodsDialog::~GoodsDialog()
{
}

void GoodsDialog::loadTable(bool includeDeleted)
{
    clearTable();
    
    if (!m_core || !m_core->getDatabaseConnection())
        return;

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);
    q.setForwardOnly(true);
    
    QString sql = QStringLiteral(
        "SELECT g.id, g.name, g.description, c.name, g.isactive "
        "FROM goods g "
        "LEFT JOIN goodcats c ON g.categoryid = c.id");
    
    if (!includeDeleted)
        sql += QStringLiteral(" WHERE (g.isdeleted IS NULL OR g.isdeleted = false)");
    
    sql += QStringLiteral(" ORDER BY g.name");
    
    if (!q.exec(sql)) {
        showError(tr("Ошибка загрузки: %1").arg(q.lastError().text()));
        return;
    }

    while (q.next()) {
        const int row = insertRow();
        setCellText(row, 0, q.value(0).toString());  // ID
        setCellText(row, 1, q.value(1).toString());  // Название
        setCellText(row, 2, q.value(2).toString());  // Описание
        setCellText(row, 3, q.value(3).toString());  // Категория
        setCellText(row, 4, q.value(4).toBool() ? tr("Да") : tr("Нет"));  // Активен
    }
    
    updateRecordCount();
}

void GoodsDialog::addRecord()
{
    addOrEditGood(0);
}

void GoodsDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0) {
        showWarning(tr("Выберите товар для редактирования."));
        return;
    }
    addOrEditGood(cellText(row, 0).toInt());
}

void GoodsDialog::addOrEditGood(int existingId)
{
    QString name, description;
    int categoryId = 1;
    bool isActive = true;

    if (existingId > 0) {
        const int row = currentRow();
        if (row < 0) return;
        name = cellText(row, 1);
        description = cellText(row, 2);
        isActive = (cellText(row, 4) == tr("Да"));
        QSqlQuery sq(m_core->getDatabaseConnection()->getDatabase());
        sq.prepare(QStringLiteral("SELECT categoryid FROM goods WHERE id = :id"));
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

    UserSession session = m_core->getCurrentSession();
    qint64 employeeId = m_core->getEmployeeIdByUserId(session.userId);
    if (employeeId <= 0) {
        showError(tr("Нет привязки пользователя к сотруднику."));
        return;
    }

    GoodEditDialog dlg(this);
    dlg.setCategories(catIds, catNames);
    dlg.setData(name, description, categoryId, isActive);
    dlg.setWindowTitle(existingId > 0 ? tr("Редактирование товара") : tr("Новый товар"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    name = dlg.name();
    description = dlg.description();
    categoryId = dlg.categoryId();
    isActive = dlg.isActive();

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);

    if (existingId > 0) {
        q.prepare(QStringLiteral(
            "UPDATE goods SET name = :name, description = :desc, categoryid = :catid, "
            "isactive = :active, updatedate = :ud WHERE id = :id"));
        q.bindValue(QStringLiteral(":name"), name);
        q.bindValue(QStringLiteral(":desc"), description);
        q.bindValue(QStringLiteral(":catid"), categoryId);
        q.bindValue(QStringLiteral(":active"), isActive);
        q.bindValue(QStringLiteral(":ud"), QDate::currentDate());
        q.bindValue(QStringLiteral(":id"), existingId);
        if (!q.exec()) {
            showError(tr("Ошибка сохранения: %1").arg(q.lastError().text()));
            return;
        }
        showInfo(tr("Товар обновлён."));
    } else {
        q.prepare(QStringLiteral(
            "INSERT INTO goods (name, description, categoryid, isactive, updatedate, employeeid, isdeleted) "
            "VALUES (:name, :desc, :catid, :active, :ud, :empid, false) RETURNING id"));
        q.bindValue(QStringLiteral(":name"), name);
        q.bindValue(QStringLiteral(":desc"), description);
        q.bindValue(QStringLiteral(":catid"), categoryId);
        q.bindValue(QStringLiteral(":active"), isActive);
        q.bindValue(QStringLiteral(":ud"), QDate::currentDate());
        q.bindValue(QStringLiteral(":empid"), employeeId);
        if (!q.exec() || !q.next()) {
            showError(tr("Ошибка добавления: %1").arg(q.lastError().text()));
            return;
        }
        showInfo(tr("Товар добавлен."));
    }
    loadTable();
}

void GoodsDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;

    const int id = cellText(row, 0).toInt();
    const QString name = cellText(row, 1);
    
    if (!askConfirmation(tr("Пометить товар «%1» как удалённый?").arg(name)))
        return;

    if (!m_core || !m_core->getDatabaseConnection())
        return;

    QSqlDatabase db = m_core->getDatabaseConnection()->getDatabase();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE goods SET isdeleted = true WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    
    if (!q.exec()) {
        showError(tr("Не удалось удалить: %1").arg(q.lastError().text()));
        return;
    }
    
    showInfo(tr("Товар помечен как удалённый."));
    loadTable();
}
