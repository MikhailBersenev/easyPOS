#include "goodsdialog.h"
#include "../easyposcore.h"
#include "../db/databaseconnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>

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
    showInfo(tr("Функция добавления товара — в разработке.\n"
                "Нужен полноценный диалог с выбором категории, загрузкой изображения и т.д."));
}

void GoodsDialog::editRecord()
{
    showInfo(tr("Функция редактирования товара — в разработке.\n"
                "Нужен полноценный диалог с выбором категории, загрузкой изображения и т.д."));
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
