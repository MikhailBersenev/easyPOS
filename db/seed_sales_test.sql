-- Тестовые данные для проверки SalesManager (продажи).
-- Запускать на копии БД или после очистки. При повторном запуске возможны конфликты по PK.
-- База: pos_bakery (схема из дампа).

BEGIN;

-- 1. Должность (если ещё нет)
INSERT INTO positions (id, name, isactive, updatedate, isdeleted)
VALUES (1, 'Кассир', true, CURRENT_DATE, false)
ON CONFLICT (id) DO NOTHING;

-- 2. Сотрудник (userid = 1 → пользователь root, иначе касса не откроется)
INSERT INTO employees (id, lastname, firstname, middlename, positionid, userid, updatedate, isdeleted)
VALUES (1, 'Тестов', 'Иван', 'Иванович', 1, 1, CURRENT_DATE, false)
ON CONFLICT (id) DO UPDATE SET userid = 1;

-- 3. Категория товаров
INSERT INTO goodcats (id, name, description, updatedate, isdeleted)
VALUES (1, 'Выпечка', 'Хлеб и выпечка', CURRENT_DATE, false)
ON CONFLICT (id) DO NOTHING;

-- 4. Товар (employeeid — кто добавил)
INSERT INTO goods (id, name, description, categoryid, imageurl, isactive, discountid, updatedate, employeeid, isdeleted)
VALUES (1, 'Хлеб белый', 'Белый хлеб 0.5 кг', 1, NULL, true, NULL, CURRENT_DATE, 1, false)
ON CONFLICT (id) DO NOTHING;

-- 5. Партия товара (goodid, qnt, price, proddate, expdate, emoloyeeid — кто принял)
INSERT INTO batches (id, goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price)
VALUES (
    1,
    1,                    -- goodid
    100,                  -- qnt
    'BATCH-001',
    CURRENT_DATE,
    CURRENT_DATE + 7,     -- срок годности +7 дней
    false,
    CURRENT_DATE,
    1,                    -- emoloyeeid
    false,
    55.00                 -- price
)
ON CONFLICT (id) DO NOTHING;

-- 6. Ставка НДС (если пусто — SalesManager создаёт «Без НДС» сам; для ручных INSERT нужна хотя бы одна)
INSERT INTO vatrates (id, name, rate, isdeleted)
VALUES (1, 'Без НДС', 0, false)
ON CONFLICT (id) DO NOTHING;

-- 7. Номенклатура (item) для партии — ссылка на batches
INSERT INTO items (id, batchid, serviceid, isdeleted)
VALUES (1, 1, NULL, false)
ON CONFLICT (id) DO NOTHING;

COMMIT;

-- =============================================================================
-- Пример чека и позиций (опционально). Раскомментируй и выполни отдельно.
-- Либо создавай чеки через SalesManager в приложении.
-- =============================================================================

/*
BEGIN;

-- Чек
INSERT INTO checks (date, "time", totalamount, discountamount, employeeid, isdeleted)
VALUES (CURRENT_DATE, CURRENT_TIME, 0, 0, 1, false);

-- Подставь id чека (например из RETURNING). Ниже — для чека с id = 1.
-- Если только что создали чек, используй: SELECT id FROM checks ORDER BY id DESC LIMIT 1;

-- Позиция 1: 2 шт. хлеба по 55 — sum = 110
INSERT INTO sales (itemid, qnt, vatrateid, sum, comment, checkid, isdeleted)
VALUES (1, 2, 1, 110.00, NULL, 1, false);

-- Списываем с партии
UPDATE batches SET qnt = qnt - 2 WHERE id = 1;

-- Итог чека
UPDATE checks SET totalamount = 110.00 WHERE id = 1;

COMMIT;
*/

-- =============================================================================
-- Проверка через SalesManager (в приложении)
-- =============================================================================
--
--   SalesManager* sm = core->createSalesManager(this);
--
--   sm->createCheck(1);                    -- employeeid = 1
--   sm->addSaleByBatchId(checkId, 1, 2);  -- чек, batchid=1, qnt=2  →  sum = 110
--   sm->setCheckDiscount(checkId, 10.0);  -- скидка 10 ₽
--   QList<SaleRow> rows = sm->getSalesByCheck(checkId);
--   sm->getChecks(QDate::currentDate(), QDate::currentDate());
--
-- Удаление позиции / отмена чека вернёт qnt обратно в партию.
--
