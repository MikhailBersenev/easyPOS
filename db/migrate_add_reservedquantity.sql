-- Миграция: добавление поля reservedquantity в таблицу batches
-- Для поддержки резервирования товаров в StockManager
-- База: pos_bakery

BEGIN;

-- Добавляем поле reservedquantity (по умолчанию 0)
ALTER TABLE batches ADD COLUMN IF NOT EXISTS reservedquantity bigint DEFAULT 0 NOT NULL;

-- Комментарий к полю
COMMENT ON COLUMN batches.reservedquantity IS 'Зарезервированное количество в партии';

COMMIT;
