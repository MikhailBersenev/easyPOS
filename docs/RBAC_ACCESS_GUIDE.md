# Методичка: разграничение доступа по ролям в easyPOS

## 1. Обзор

В приложении easyPOS доступ к функциям ограничивается по **ролям** и их **уровням доступа**. Уровень — это число: чем оно **меньше**, тем **больше** прав (0 — максимальные права).

- **Администратор (Admin)** — уровень 0: полный доступ.
- **Менеджер (Manager)** — уровень 10: справочники, производство, отчёты, история чеков, касса.
- **Кассир (Cashier)** — уровень 50: касса, история чеков, сохранение чека в PDF, настройка «Печатать после оплаты».

Реализация состоит из двух частей:

1. **Скрытие UI** — пункты меню и кнопки скрываются, если у роли нет нужного уровня.
2. **Проверки в коде** — в начале обработчиков действий выполняется проверка уровня; при недостаточных правах показывается предупреждение и действие не выполняется (защита от обхода UI).

---

## 2. Уровни доступа (код)

Файл: `RBAC/structures.h`

```cpp
namespace AccessLevel {
    constexpr int Admin   = 0;   // Всё: настройки, БД, пользователи, справочники, производство, отчёты, касса
    constexpr int Manager = 10;  // Справочники, производство, отчёты, история чеков, настройка печати, касса
    constexpr int Cashier = 50;  // Только касса, история чеков, сохранение чека в PDF
}
```

Правило: доступ к действию с «требуемым уровнем» N имеют все роли, у которых `role.level <= N`.  
Пример: требование `AccessLevel::Manager` (10) пропускает Admin (0) и Manager (10), не пропускает Cashier (50).

---

## 3. Роль и проверка уровня

В `RBAC/structures.h` структура `Role` содержит поле `level` и метод:

```cpp
/// Доступ разрешён, если уровень роли не выше (число не больше) требуемого
bool hasAccessLevel(int requiredLevel) const {
    return isActive && !isBlocked && level <= requiredLevel;
}
```

Использование в коде:

- `m_session.role.hasAccessLevel(AccessLevel::Admin)` — только администратор.
- `m_session.role.hasAccessLevel(AccessLevel::Manager)` — администратор или менеджер.
- `m_session.role.hasAccessLevel(AccessLevel::Cashier)` — все роли с доступом к кассе (фактически все, у кого роль активна и не заблокирована при текущих уровнях).

Сессия пользователя хранится в `MainWindow` в `m_session` (тип `UserSession`); в ней есть `m_session.role`.

---

## 4. Применение в главном окне (MainWindow)

### 4.1. Когда вызывается

В конструкторе `MainWindow` после инициализации UI вызывается:

```cpp
applyRoleAccess();
```

Он один раз выставляет видимость пунктов меню и кнопок в зависимости от роли текущего пользователя.

### 4.2. Что делает `applyRoleAccess()`

Файл: `mainwindow.cpp`, метод `MainWindow::applyRoleAccess()`.

Вычисляются флаги:

- `isAdmin`   = `role.hasAccessLevel(AccessLevel::Admin)`
- `isManager` = `role.hasAccessLevel(AccessLevel::Manager)`
- `isCashier` = `role.hasAccessLevel(AccessLevel::Cashier)`

По ним выставляется видимость действий (меню/панель инструментов):

| Действие | Видимость |
|----------|-----------|
| Настройки (общие) | только Admin |
| Подключение к БД | только Admin |
| Печатать чек после оплаты | Cashier и выше (все) |
| Категории, Товары, Услуги, Ставки НДС, Остатки | Manager и выше |
| Рецепты, Запуск производства | Manager и выше |
| История чеков, Отчёт по продажам | Manager и выше |
| Сохранить чек в PDF | Cashier и выше |
| Новый чек, Поиск товара, Оплатить | Cashier и выше |

Остальные пункты (Выход, О программе, Горячие клавиши и т.п.) в этом методе не трогаются и доступны всем.

---

## 5. Проверки в обработчиках действий

Для действий, ограниченных по ролям, в начале соответствующего слота делается:

1. Проверка сессии: `if (!m_core || !m_core->ensureSessionValid()) { close(); return; }`
2. Проверка уровня:  
   `if (!m_session.role.hasAccessLevel(ТребуемыйУровень)) { QMessageBox::warning(...); return; }`
3. Далее — обычная логика открытия диалога или выполнения действия.

Пример (справочники и отчёты — уровень Manager):

```cpp
void MainWindow::on_actionCategories_triggered()
{
    if (!m_core || !m_core->ensureSessionValid()) { close(); return; }
    if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для справочников."));
        return;
    }
    // ... открытие диалога
}
```

Пример (только администратор):

```cpp
void MainWindow::on_actionDbConnection_triggered()
{
    if (!m_core) return;
    if (!m_session.role.hasAccessLevel(AccessLevel::Admin)) {
        QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав. Только администратор."));
        return;
    }
    // ...
}
```

Где какие проверки стоят (кратко):

- **Admin:** `on_actionSettings_triggered`, `on_actionDbConnection_triggered`
- **Manager:** `on_actionCheckHistory_triggered`, `on_actionSalesReport_triggered`, `on_actionRecipes_triggered`, `on_actionProductionRun_triggered`, `on_actionStockBalance_triggered`, `on_actionCategories_triggered`, `on_actionGoods_triggered`, `on_actionServices_triggered`, `on_actionVatRates_triggered`
- **Cashier:** `on_actionSaveCheckToPdf_triggered` (остальные кассовые действия доступны всем вошедшим пользователям при видимых кнопках)

---

## 6. Матрица доступа (кто что может)

| Функция | Admin | Manager | Cashier |
|---------|-------|---------|---------|
| Настройки (общие) | ✓ | ✗ | ✗ |
| Подключение к БД | ✓ | ✗ | ✗ |
| Категории, Товары, Услуги, НДС, Остатки | ✓ | ✓ | ✗ |
| Рецепты, Запуск производства | ✓ | ✓ | ✗ |
| История чеков, Отчёт по продажам | ✓ | ✓ | ✗ |
| Печатать чек после оплаты | ✓ | ✓ | ✓ |
| Новый чек, Поиск, Оплата, Скидка и т.д. | ✓ | ✓ | ✓ |
| Сохранить чек в PDF | ✓ | ✓ | ✓ |
| Выход, О программе, Горячие клавиши | ✓ | ✓ | ✓ |

---

## 7. Как добавить новое ограниченное действие

1. **Определить требуемый уровень**  
   Решить, кому доступно: только Admin, Manager и выше или все (Cashier и выше). Выбрать константу из `AccessLevel`.

2. **В `MainWindow::applyRoleAccess()`**  
   Найти нужный `QAction` (пункт меню или кнопку) и выставить видимость, например:
   - только админ: `ui->actionX->setVisible(isAdmin);`
   - менеджер и выше: `ui->actionX->setVisible(isManager);`
   - кассир и выше: `ui->actionX->setVisible(isCashier);`

3. **В обработчике слота**  
   В начале обработчика после проверки сессии добавить:
   ```cpp
   if (!m_session.role.hasAccessLevel(AccessLevel::Manager)) {  // или Admin / Cashier
       QMessageBox::warning(this, tr("Доступ"), tr("Недостаточно прав для ..."));
       return;
   }
   ```

4. **Роль в БД**  
   Убедиться, что в таблице `roles` у записей (admin, manager, cashier) заданы корректные значения поля `level`: 0, 10, 50. При создании новых ролей задавать им подходящий `level`.

---

## 8. Важные файлы

| Назначение | Файл |
|------------|------|
| Уровни и структуры RBAC | `RBAC/structures.h` |
| Видимость меню/кнопок и проверки в слотах | `mainwindow.cpp` (applyRoleAccess, on_action*_triggered) |
| Заголовок главного окна | `mainwindow.h` |

---

## 9. Расширение системы (если понадобится)

- **Новые уровни:** добавить константы в `AccessLevel` (например, 5 для «суперменеджера») и использовать их в `applyRoleAccess()` и в проверках слотов.
- **Разрешения (permissions):** в `Role` уже есть заготовка `hasPermission(const QString &)`, сейчас она не используется; логика построена на уровнях. При переходе на детальные разрешения нужно будет загружать их из БД и проверять в коде вместо или вместе с `hasAccessLevel`.

Методичка актуальна для текущей реализации разграничения доступа в easyPOS.
