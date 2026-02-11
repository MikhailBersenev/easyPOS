#ifndef REFERENCEDIALOG_H
#define REFERENCEDIALOG_H

#include <QDialog>
#include <memory>

class EasyPOSCore;

namespace Ui {
class ReferenceDialog;
}

/**
 * @brief Базовый класс для диалогов справочников
 * 
 * Предоставляет стандартный интерфейс: таблицу и кнопки Добавить/Изменить/Удалить/Закрыть.
 * Производные классы переопределяют виртуальные методы для специфической логики.
 */
class ReferenceDialog : public QDialog
{
    Q_OBJECT

signals:
    void reportButtonClicked();

public:
    explicit ReferenceDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core, 
                            const QString &title);
    ~ReferenceDialog();

protected:
    // Виртуальные методы для переопределения в производных классах
    
    /**
     * @brief Загрузка данных в таблицу
     * @param includeDeleted - показывать ли удалённые записи
     */
    virtual void loadTable(bool includeDeleted = false) = 0;
    
    /**
     * @brief Добавление новой записи
     */
    virtual void addRecord() = 0;
    
    /**
     * @brief Редактирование выбранной записи
     */
    virtual void editRecord() = 0;
    
    /**
     * @brief Удаление выбранной записи
     */
    virtual void deleteRecord() = 0;
    
    /**
     * @brief Настройка колонок таблицы (вызывается в конструкторе производного класса)
     * @param columns - список заголовков колонок
     * @param widths - список ширин колонок (опционально)
     */
    void setupColumns(const QStringList &columns, const QList<int> &widths = QList<int>());
    
    /**
     * @brief Получить текущую выбранную строку
     * @return номер строки или -1 если ничего не выбрано
     */
    int currentRow() const;
    
    /**
     * @brief Получить значение ячейки
     */
    QString cellText(int row, int column) const;
    
    /**
     * @brief Установить значение ячейки
     */
    void setCellText(int row, int column, const QString &text);
    
    /**
     * @brief Добавить строку в таблицу
     * @return номер добавленной строки
     */
    int insertRow();
    
    /**
     * @brief Очистить таблицу
     */
    void clearTable();
    
    /**
     * @brief Показать предупреждение
     */
    void showWarning(const QString &message);
    
    /**
     * @brief Показать ошибку
     */
    void showError(const QString &message);
    
    /**
     * @brief Показать информацию
     */
    void showInfo(const QString &message);
    
    /**
     * @brief Запросить подтверждение
     */
    bool askConfirmation(const QString &message);
    
    /**
     * @brief Обновить счётчик записей
     */
    void updateRecordCount();

    /** Показать кнопку «Отчёт» (для справочников с экспортом отчёта) */
    void setReportButtonVisible(bool visible);
    /** Текст кнопки отчёта */
    void setReportButtonText(const QString &text);

    // Доступ к core для производных классов
    std::shared_ptr<EasyPOSCore> m_core;

private slots:
    void on_addButton_clicked();
    void on_editButton_clicked();
    void on_deleteButton_clicked();
    void on_closeButton_clicked();
    void on_refreshButton_clicked();
    void on_searchButton_clicked();
    void on_searchEdit_textChanged(const QString &text);
    void on_searchEdit_returnPressed();
    void on_tableWidget_doubleClicked(const QModelIndex &index);
    void on_tableWidget_customContextMenuRequested(const QPoint &pos);

private:
    void setupShortcuts();
    void applyFilter(const QString &searchText);
    
    Ui::ReferenceDialog *ui;
    QString m_lastSearchText;
};

#endif // REFERENCEDIALOG_H
