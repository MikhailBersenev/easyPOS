# Справочники — Базовый класс ReferenceDialog

## Архитектура

Все диалоги справочников наследуются от базового класса **`ReferenceDialog`**, который предоставляет:
- **Стандартный UI**: таблица + кнопки (Добавить, Изменить, Удалить, Обновить, Закрыть)
- **Поиск/фильтрация** по всем колонкам таблицы
- **Чекбокс "Показать удалённые"** для отображения удалённых записей
- **Счётчик записей** с учётом фильтрации
- **Горячие клавиши**: Insert (добавить), Enter (редактировать), Delete (удалить), F5 (обновить), Esc (закрыть)
- **Двойной клик** по строке для редактирования
- **Контекстное меню** (ПКМ) с операциями
- Виртуальные методы для переопределения логики CRUD
- Утилитные методы для работы с таблицей и диалогами

## Создание нового справочника

### 1. Создайте класс, наследующий ReferenceDialog

```cpp
// ui/myreferencedialog.h
#ifndef MYREFERENCEDIALOG_H
#define MYREFERENCEDIALOG_H

#include "referencedialog.h"

class MyReferenceDialog : public ReferenceDialog
{
    Q_OBJECT

public:
    explicit MyReferenceDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core);
    ~MyReferenceDialog();

protected:
    void loadTable(bool includeDeleted = false) override;
    void addRecord() override;
    void editRecord() override;
    void deleteRecord() override;
};

#endif
```

### 2. Реализуйте виртуальные методы

```cpp
// ui/myreferencedialog.cpp
#include "myreferencedialog.h"

MyReferenceDialog::MyReferenceDialog(QWidget *parent, std::shared_ptr<EasyPOSCore> core)
    : ReferenceDialog(parent, core, tr("Мой справочник"))
{
    // Настройка колонок: заголовки и ширины
    setupColumns({tr("ID"), tr("Название"), tr("Описание")}, 
                 {50, 200, 300});
    
    // Загрузка данных
    loadTable();
}

MyReferenceDialog::~MyReferenceDialog()
{
}

void MyReferenceDialog::loadTable(bool includeDeleted)
{
    clearTable();  // Очистка таблицы
    
    // Загрузка данных из БД через менеджер
    // ...
    
    // Добавление строк:
    const int row = insertRow();
    setCellText(row, 0, "123");
    setCellText(row, 1, "Название");
    setCellText(row, 2, "Описание");
}

void MyReferenceDialog::addRecord()
{
    // Логика добавления новой записи
    // После добавления вызвать loadTable() для обновления
}

void MyReferenceDialog::editRecord()
{
    const int row = currentRow();  // Получить выбранную строку
    if (row < 0) return;
    
    // Получить данные из ячеек:
    QString id = cellText(row, 0);
    QString name = cellText(row, 1);
    
    // Редактирование...
    // После изменения вызвать loadTable()
}

void MyReferenceDialog::deleteRecord()
{
    const int row = currentRow();
    if (row < 0) return;
    
    QString name = cellText(row, 1);
    if (!askConfirmation(tr("Удалить запись «%1»?").arg(name)))
        return;
    
    // Удаление из БД...
    loadTable();
}
```

### 3. Добавьте в CMakeLists.txt

```cmake
ui/myreferencedialog.h ui/myreferencedialog.cpp
```

### 4. Подключите в MainWindow

```cpp
#include "ui/myreferencedialog.h"

void MainWindow::on_actionMyReference_triggered()
{
    if (!m_core) return;
    MyReferenceDialog *dlg = WindowCreator::Create<MyReferenceDialog>(this, m_core, false);
    if (dlg)
        dlg->exec();
}
```

## Доступные утилитные методы

### Работа с таблицей
- `setupColumns(QStringList columns, QList<int> widths)` — настройка колонок
- `currentRow()` — номер выбранной строки
- `cellText(row, column)` — получить текст ячейки
- `setCellText(row, column, text)` — установить текст ячейки
- `insertRow()` — добавить новую строку (возвращает номер)
- `clearTable()` — очистить таблицу
- `updateRecordCount()` — обновить счётчик записей (вызывайте после loadTable)
- `showDeleted()` — получить состояние чекбокса "Показать удалённые"

### Диалоги
- `showWarning(message)` — предупреждение
- `showError(message)` — ошибка
- `showInfo(message)` — информация
- `askConfirmation(message)` — вопрос Да/Нет

### Доступ к core
- `m_core` — `std::shared_ptr<EasyPOSCore>` доступен в производных классах

## Встроенные возможности

### Поиск и фильтрация
Поиск работает автоматически по всем колонкам таблицы. Просто введите текст в поле "Поиск" — строки, не содержащие искомый текст, будут скрыты.

### Горячие клавиши
- **Insert** — добавить запись
- **Enter** — редактировать выбранную запись
- **Delete** — удалить выбранную запись
- **F5** — обновить данные из БД
- **Esc** — закрыть окно

### Двойной клик
Двойной клик по строке открывает форму редактирования (аналогично Enter).

### Контекстное меню (ПКМ)
Правый клик по таблице открывает меню с операциями: Добавить, Изменить, Удалить, Обновить.

## Примеры

### Категории товаров (GoodCatsDialog)
- Использует `CategoryManager` для CRUD операций
- Редактирование через `QInputDialog` (упрощённый вариант)
- Мягкое удаление (флаг `isdeleted`)

### Товары (GoodsDialog)
- Загрузка данных напрямую через `QSqlQuery`
- Отображение связанных данных (категория через JOIN)
- Заглушки для добавления/редактирования (требуют отдельных форм)

## Рекомендации

1. **Для простых справочников** (1-3 поля): используйте `QInputDialog` как в `GoodCatsDialog`
2. **Для сложных справочников**: создайте отдельный диалог редактирования записи
3. **Менеджеры данных**: создавайте отдельные Manager-классы (как `CategoryManager`) для инкапсуляции работы с БД
4. **Обновление таблицы**: всегда вызывайте `loadTable()` после изменения данных

## Преимущества подхода

✅ **Нет дублирования кода** — вся логика UI и кнопок в базовом классе  
✅ **Единообразие** — все справочники выглядят и работают одинаково  
✅ **Простота добавления** — новый справочник = ~50 строк кода  
✅ **Легко расширять** — можно добавить общие фичи (фильтры, экспорт) в базовый класс  
✅ **Богатый функционал** — поиск, горячие клавиши, контекстное меню из коробки  
✅ **UX** — двойной клик, подсказки, счётчик записей, чередующиеся цвета строк
