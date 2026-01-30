-- Добавление товаров, категорий, партий и номенклатуры.
-- Запускать после seed_sales_test.sql (нужны employees.id = 1, goodcats).
-- База: pos_bakery.
--
-- Порядок: psql ... -f db/seed_sales_test.sql && psql ... -f db/seed_goods.sql
--
-- Чтобы добавить свои товары:
-- 1. goodcats — категория (id, name, description, updatedate, isdeleted).
-- 2. goods — товар (name, description, categoryid, employeeid, updatedate, isactive, isdeleted, imageurl, discountid).
-- 3. batches — партия (goodid, qnt, price, proddate, expdate, emoloyeeid, updatedate, batchnumber, writtenoff, isdeleted).
-- 4. items — номенклатура (batchid, serviceid = NULL, isdeleted). Либо касса создаст при первой продаже.

BEGIN;

-- Категории (если ещё нет)
INSERT INTO goodcats (id, name, description, updatedate, isdeleted)
VALUES
  (2, 'Напитки', 'Чай, кофе, соки', CURRENT_DATE, false),
  (3, 'Сладости', 'Печенье, конфеты', CURRENT_DATE, false)
ON CONFLICT (id) DO NOTHING;

-- Товары (categoryid, employeeid — кто добавил)
INSERT INTO goods (id, name, description, categoryid, imageurl, isactive, discountid, updatedate, employeeid, isdeleted)
VALUES
  (2, 'Хлеб ржаной', 'Ржаной хлеб 0.5 кг', 1, NULL, true, NULL, CURRENT_DATE, 1, false),
  (3, 'Булочка с изюмом', 'Сдобная булочка 80 г', 1, NULL, true, NULL, CURRENT_DATE, 1, false),
  (4, 'Круассан', 'Круассан 60 г', 1, NULL, true, NULL, CURRENT_DATE, 1, false),
  (5, 'Чай чёрный', 'Чай чёрный 25 пак', 2, NULL, true, NULL, CURRENT_DATE, 1, false),
  (6, 'Кофе молотый', 'Кофе 250 г', 2, NULL, true, NULL, CURRENT_DATE, 1, false),
  (7, 'Печенье овсяное', 'Печенье 200 г', 3, NULL, true, NULL, CURRENT_DATE, 1, false),
  (8, 'Шоколад тёмный', 'Шоколад 100 г', 3, NULL, true, NULL, CURRENT_DATE, 1, false)
ON CONFLICT (id) DO NOTHING;

-- Партии (goodid, qnt, price, proddate, expdate, emoloyeeid)
INSERT INTO batches (id, goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price)
VALUES
  (2, 2, 50,  'BATCH-002', CURRENT_DATE, CURRENT_DATE + 5,  false, CURRENT_DATE, 1, false, 48.00),
  (3, 3, 80,  'BATCH-003', CURRENT_DATE, CURRENT_DATE + 3,  false, CURRENT_DATE, 1, false, 35.00),
  (4, 4, 60,  'BATCH-004', CURRENT_DATE, CURRENT_DATE + 2,  false, CURRENT_DATE, 1, false, 55.00),
  (5, 5, 30,  'BATCH-005', CURRENT_DATE, CURRENT_DATE + 365, false, CURRENT_DATE, 1, false, 120.00),
  (6, 6, 20,  'BATCH-006', CURRENT_DATE, CURRENT_DATE + 180, false, CURRENT_DATE, 1, false, 280.00),
  (7, 7, 40,  'BATCH-007', CURRENT_DATE, CURRENT_DATE + 90,  false, CURRENT_DATE, 1, false, 95.00),
  (8, 8, 25,  'BATCH-008', CURRENT_DATE, CURRENT_DATE + 180, false, CURRENT_DATE, 1, false, 150.00)
ON CONFLICT (id) DO NOTHING;

-- Номенклатура (items) для партий — касса ищет/создаёт по batchid, но можно добавить заранее
INSERT INTO items (id, batchid, serviceid, isdeleted)
VALUES
  (2, 2, NULL, false),
  (3, 3, NULL, false),
  (4, 4, NULL, false),
  (5, 5, NULL, false),
  (6, 6, NULL, false),
  (7, 7, NULL, false),
  (8, 8, NULL, false)
ON CONFLICT (id) DO NOTHING;

COMMIT;

-- Обновить сиквенсы для следующих INSERT без явного id
SELECT setval('goodcats_id_seq', (SELECT COALESCE(MAX(id), 1) FROM goodcats));
SELECT setval('goods_id_seq',   (SELECT COALESCE(MAX(id), 1) FROM goods));
SELECT setval('batches_id_seq', (SELECT COALESCE(MAX(id), 1) FROM batches));
SELECT setval('items_id_seq',   (SELECT COALESCE(MAX(id), 1) FROM items));

-- Проверка: SELECT g.name, b.price, b.qnt FROM goods g JOIN batches b ON b.goodid = g.id WHERE b.qnt > 0;
