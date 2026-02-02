#include "goodcatsdialog.h"
#include "../easyposcore.h"
#include "../goods/categorymanager.h"
#include "../goods/structures.h"
#include <QInputDialog>

GoodCatsDialog::GoodCatsDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Категории товаров"))
{
    // Настройка колонок таблицы
    setupColumns({tr("ID"), tr("Название"), tr("Описание"), tr("Дата")},
                 {50, 200, 300, 120});
    
    // Загрузка данных
    loadTable();
}

GoodCatsDialog::~GoodCatsDialog()
{
}

void GoodCatsDialog::loadTable(bool includeDeleted)
{
    clearTable();
    
    if (!m_core || !m_core->getDatabaseConnection())
        return;

    CategoryManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    const QList<GoodCategory> list = mgr.list(includeDeleted);

    for (const GoodCategory &cat : list) {
        const int row = insertRow();
        setCellText(row, 0, QString::number(cat.id));
        setCellText(row, 1, cat.name);
        setCellText(row, 2, cat.description);
        setCellText(row, 3, cat.updateDate.toString(Qt::ISODate));
    }
    
    updateRecordCount();
}

void GoodCatsDialog::addRecord()
{
    addOrEditCategory(nullptr);
}

void GoodCatsDialog::editRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;

    GoodCategory cat;
    cat.id = cellText(row, 0).toInt();
    cat.name = cellText(row, 1);
    cat.description = cellText(row, 2);
    
    addOrEditCategory(&cat);
}

void GoodCatsDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0)
        return;

    const int id = cellText(row, 0).toInt();
    const QString name = cellText(row, 1);
    
    if (!askConfirmation(tr("Пометить категорию «%1» как удалённую?").arg(name)))
        return;

    CategoryManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());
    
    if (!mgr.setDeleted(id, true)) {
        showError(tr("Не удалось удалить: %1").arg(mgr.lastError().text()));
        return;
    }
    
    showInfo(tr("Категория помечена как удалённая."));
    loadTable();
}

void GoodCatsDialog::addOrEditCategory(const GoodCategory *existing)
{
    bool okName = false, okDesc = false;
    
    QString name = existing
        ? QInputDialog::getText(this, tr("Категория"), tr("Название:"),
                                QLineEdit::Normal, existing->name, &okName)
        : QInputDialog::getText(this, tr("Новая категория"), tr("Название:"),
                                QLineEdit::Normal, QString(), &okName);
    if (!okName || name.trimmed().isEmpty())
        return;

    QString description = existing
        ? QInputDialog::getText(this, tr("Категория"), tr("Описание:"),
                                QLineEdit::Normal, existing->description, &okDesc)
        : QInputDialog::getText(this, tr("Новая категория"), tr("Описание:"),
                                QLineEdit::Normal, QString(), &okDesc);
    if (!okDesc)
        return;

    CategoryManager mgr(this);
    mgr.setDatabaseConnection(m_core->getDatabaseConnection());

    if (existing) {
        // Редактирование
        GoodCategory cat = *existing;
        cat.name = name.trimmed();
        cat.description = description.trimmed();
        
        if (!mgr.update(cat)) {
            showError(tr("Не удалось сохранить: %1").arg(mgr.lastError().text()));
            return;
        }
        showInfo(tr("Категория обновлена."));
    } else {
        // Добавление
        GoodCategory cat;
        cat.name = name.trimmed();
        cat.description = description.trimmed();
        
        if (!mgr.add(cat)) {
            showError(tr("Не удалось добавить: %1").arg(mgr.lastError().text()));
            return;
        }
        showInfo(tr("Категория добавлена."));
    }
    
    loadTable();
}
