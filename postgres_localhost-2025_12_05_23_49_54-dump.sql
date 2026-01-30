--
-- PostgreSQL database dump
--

\restrict edvoeSmz5ApTYwvGHut85GfDRaf0K876FIzdm9IdHk4bcEywW3AMMrArboJTTir

-- Dumped from database version 18.1
-- Dumped by pg_dump version 18.1

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: barcodes; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.barcodes (
    id bigint NOT NULL,
    barcode text NOT NULL,
    batchid bigint NOT NULL,
    updatedate date NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.barcodes OWNER TO postgres;

--
-- Name: TABLE barcodes; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.barcodes IS 'Штрихкоды (EAN/UPC)';


--
-- Name: COLUMN barcodes.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.barcodes.id IS 'Первичный ключ';


--
-- Name: COLUMN barcodes.barcode; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.barcodes.barcode IS 'Штрихкод';


--
-- Name: COLUMN barcodes.batchid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.barcodes.batchid IS 'ID партии';


--
-- Name: COLUMN barcodes.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.barcodes.updatedate IS 'Дата последнего изменения';


--
-- Name: COLUMN barcodes.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.barcodes.isdeleted IS 'Флаг удаления';


--
-- Name: barcodes_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.barcodes_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.barcodes_id_seq OWNER TO postgres;

--
-- Name: barcodes_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.barcodes_id_seq OWNED BY public.barcodes.id;


--
-- Name: batches; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.batches (
    id bigint NOT NULL,
    goodid bigint NOT NULL,
    qnt bigint NOT NULL,
    batchnumber text,
    proddate date NOT NULL,
    expdate date NOT NULL,
    writtenoff boolean,
    updatedate date NOT NULL,
    emoloyeeid bigint NOT NULL,
    isdeleted boolean,
    price double precision NOT NULL
);


ALTER TABLE public.batches OWNER TO postgres;

--
-- Name: TABLE batches; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.batches IS 'Партии товаров';


--
-- Name: COLUMN batches.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.id IS 'Первичный ключ';


--
-- Name: COLUMN batches.qnt; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.qnt IS 'Количество на остатке';


--
-- Name: COLUMN batches.batchnumber; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.batchnumber IS 'Номер партии';


--
-- Name: COLUMN batches.proddate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.proddate IS 'Дата изготовления';


--
-- Name: COLUMN batches.expdate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.expdate IS 'Срок годности';


--
-- Name: COLUMN batches.writtenoff; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.writtenoff IS 'Списана или нет';


--
-- Name: COLUMN batches.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.updatedate IS 'Дата последнего изменения';


--
-- Name: COLUMN batches.emoloyeeid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.emoloyeeid IS 'ID сотрудника, принявшего партию';


--
-- Name: COLUMN batches.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.isdeleted IS 'Флаг удаления';


--
-- Name: COLUMN batches.price; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.batches.price IS 'Цена';


--
-- Name: batches_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.batches_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.batches_id_seq OWNER TO postgres;

--
-- Name: batches_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.batches_id_seq OWNED BY public.batches.id;


--
-- Name: checks; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.checks (
    id bigint NOT NULL,
    date date NOT NULL,
    "time" time without time zone NOT NULL,
    totalamount double precision NOT NULL,
    discountamount double precision NOT NULL,
    employeeid bigint NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.checks OWNER TO postgres;

--
-- Name: TABLE checks; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.checks IS 'Продажи (чеки)';


--
-- Name: COLUMN checks.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks.id IS 'Первичный ключ';


--
-- Name: COLUMN checks.date; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks.date IS 'Дата продажи';


--
-- Name: COLUMN checks."time"; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks."time" IS 'Время продажи';


--
-- Name: COLUMN checks.totalamount; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks.totalamount IS 'Сумма';


--
-- Name: COLUMN checks.discountamount; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks.discountamount IS 'Сумма скидки';


--
-- Name: COLUMN checks.employeeid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks.employeeid IS 'ID сотрудника, который провел продажу';


--
-- Name: COLUMN checks.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.checks.isdeleted IS 'Флаг удаления';


--
-- Name: checks_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.checks_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.checks_id_seq OWNER TO postgres;

--
-- Name: checks_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.checks_id_seq OWNED BY public.checks.id;


--
-- Name: discounts; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.discounts (
    id integer NOT NULL,
    name text NOT NULL,
    description text,
    percent double precision,
    sum double precision,
    isdeleted boolean
);


ALTER TABLE public.discounts OWNER TO postgres;

--
-- Name: TABLE discounts; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.discounts IS 'Маркетинговые акции';


--
-- Name: COLUMN discounts.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.discounts.id IS 'Первичный ключ';


--
-- Name: COLUMN discounts.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.discounts.name IS 'Название маркетинговой акции';


--
-- Name: COLUMN discounts.description; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.discounts.description IS 'Описание';


--
-- Name: COLUMN discounts.percent; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.discounts.percent IS 'Размер скидки в процентах';


--
-- Name: COLUMN discounts.sum; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.discounts.sum IS 'Размер скидки (сумма)';


--
-- Name: COLUMN discounts.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.discounts.isdeleted IS 'Флаг удаления';


--
-- Name: employees; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.employees (
    id bigint NOT NULL,
    lastname text,
    firstname text,
    middlename text,
    positionid bigint NOT NULL,
    userid bigint,
    updatedate date NOT NULL,
    isdeleted boolean NOT NULL
);


ALTER TABLE public.employees OWNER TO postgres;

--
-- Name: TABLE employees; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.employees IS 'Сотрудники';


--
-- Name: COLUMN employees.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.id IS 'Первичный ключ';


--
-- Name: COLUMN employees.lastname; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.lastname IS 'Фамилия';


--
-- Name: COLUMN employees.firstname; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.firstname IS 'Имя (основное)';


--
-- Name: COLUMN employees.middlename; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.middlename IS 'Отчество/Среднее имя';


--
-- Name: COLUMN employees.positionid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.positionid IS 'ID должности';


--
-- Name: COLUMN employees.userid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.userid IS 'ID пользователя';


--
-- Name: COLUMN employees.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.updatedate IS 'Дата  последнего изменения';


--
-- Name: COLUMN employees.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.employees.isdeleted IS 'Флаг удаления';


--
-- Name: employees_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.employees_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.employees_id_seq OWNER TO postgres;

--
-- Name: employees_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.employees_id_seq OWNED BY public.employees.id;


--
-- Name: goodcats; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.goodcats (
    id bigint NOT NULL,
    name text NOT NULL,
    description text,
    updatedate date NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.goodcats OWNER TO postgres;

--
-- Name: TABLE goodcats; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.goodcats IS 'Категории товаров';


--
-- Name: COLUMN goodcats.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goodcats.id IS 'Первичный ключ';


--
-- Name: COLUMN goodcats.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goodcats.name IS 'Название категории';


--
-- Name: COLUMN goodcats.description; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goodcats.description IS 'Описание';


--
-- Name: COLUMN goodcats.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goodcats.updatedate IS 'Дата последнего изменения';


--
-- Name: COLUMN goodcats.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goodcats.isdeleted IS 'Флаг удаления';


--
-- Name: goodcats_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.goodcats_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.goodcats_id_seq OWNER TO postgres;

--
-- Name: goodcats_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.goodcats_id_seq OWNED BY public.goodcats.id;


--
-- Name: goods; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.goods (
    id bigint NOT NULL,
    name text NOT NULL,
    description text,
    categoryid bigint NOT NULL,
    imageurl text,
    isactive boolean NOT NULL,
    discountid bigint,
    updatedate date NOT NULL,
    employeeid bigint NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.goods OWNER TO postgres;

--
-- Name: TABLE goods; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.goods IS 'Товары';


--
-- Name: COLUMN goods.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.id IS 'Первичный ключ';


--
-- Name: COLUMN goods.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.name IS 'Название товара';


--
-- Name: COLUMN goods.description; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.description IS 'Описание';


--
-- Name: COLUMN goods.categoryid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.categoryid IS 'ID категории';


--
-- Name: COLUMN goods.imageurl; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.imageurl IS 'Url картинки';


--
-- Name: COLUMN goods.isactive; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.isactive IS 'Активно или нет';


--
-- Name: COLUMN goods.discountid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.discountid IS 'ID маркетинговой акции';


--
-- Name: COLUMN goods.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.updatedate IS 'Дата последнего изменения';


--
-- Name: COLUMN goods.employeeid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.goods.employeeid IS 'ID сотрудника, добавившего товар';


--
-- Name: goods_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.goods_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.goods_id_seq OWNER TO postgres;

--
-- Name: goods_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.goods_id_seq OWNED BY public.goods.id;


--
-- Name: items; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.items (
    id bigint NOT NULL,
    batchid bigint,
    serviceid bigint,
    isdeleted boolean
);


ALTER TABLE public.items OWNER TO postgres;

--
-- Name: TABLE items; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.items IS 'Прокси таблица для товаров и услуг';


--
-- Name: COLUMN items.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.items.id IS 'Первичный ключ';


--
-- Name: COLUMN items.batchid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.items.batchid IS 'ID партии';


--
-- Name: COLUMN items.serviceid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.items.serviceid IS 'ID услуги';


--
-- Name: COLUMN items.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.items.isdeleted IS 'Флаг удаления';


--
-- Name: items_id_s; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.items_id_s
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.items_id_s OWNER TO postgres;

--
-- Name: items_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.items_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.items_id_seq OWNER TO postgres;

--
-- Name: items_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.items_id_seq OWNED BY public.items.id;


--
-- Name: positions; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.positions (
    id bigint NOT NULL,
    name text NOT NULL,
    isactive boolean NOT NULL,
    updatedate date NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.positions OWNER TO postgres;

--
-- Name: TABLE positions; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.positions IS 'Должности';


--
-- Name: COLUMN positions.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.positions.id IS 'Первичный ключ';


--
-- Name: COLUMN positions.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.positions.name IS 'Название должности';


--
-- Name: COLUMN positions.isactive; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.positions.isactive IS 'Активна или нет';


--
-- Name: COLUMN positions.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.positions.updatedate IS 'Дата последнего изменения';


--
-- Name: COLUMN positions.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.positions.isdeleted IS 'Флаг удаления';


--
-- Name: positions_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.positions_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.positions_id_seq OWNER TO postgres;

--
-- Name: positions_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.positions_id_seq OWNED BY public.positions.id;


--
-- Name: roles; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.roles (
    id bigint NOT NULL,
    name text NOT NULL,
    level integer NOT NULL,
    isblocked boolean,
    isdeleted boolean
);


ALTER TABLE public.roles OWNER TO postgres;

--
-- Name: TABLE roles; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.roles IS 'Роли';


--
-- Name: COLUMN roles.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.roles.id IS 'Первичный ключ';


--
-- Name: COLUMN roles.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.roles.name IS 'Название роли';


--
-- Name: COLUMN roles.level; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.roles.level IS 'Уровень доступа (0 - самый секретный)';


--
-- Name: COLUMN roles.isblocked; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.roles.isblocked IS 'Флаг блокировки';


--
-- Name: COLUMN roles.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.roles.isdeleted IS 'Флаг удаления';


--
-- Name: roles_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.roles_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.roles_id_seq OWNER TO postgres;

--
-- Name: roles_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.roles_id_seq OWNED BY public.roles.id;


--
-- Name: sales; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.sales (
    id bigint NOT NULL,
    itemid bigint NOT NULL,
    qnt bigint NOT NULL,
    vatrateid bigint NOT NULL,
    sum double precision NOT NULL,
    comment text,
    checkid bigint NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.sales OWNER TO postgres;

--
-- Name: TABLE sales; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.sales IS 'Продажи';


--
-- Name: COLUMN sales.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.id IS 'Первичный ключ';


--
-- Name: COLUMN sales.itemid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.itemid IS 'ID номенклатуры';


--
-- Name: COLUMN sales.qnt; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.qnt IS 'Количество проданных штук';


--
-- Name: COLUMN sales.vatrateid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.vatrateid IS 'ID ставки НДС';


--
-- Name: COLUMN sales.sum; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.sum IS 'Сумма проданных товаров без учета скидок';


--
-- Name: COLUMN sales.comment; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.comment IS 'Комментарий';


--
-- Name: COLUMN sales.checkid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.checkid IS 'ID чека';


--
-- Name: COLUMN sales.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.sales.isdeleted IS 'Флаг удаления';


--
-- Name: sales_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.sales_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.sales_id_seq OWNER TO postgres;

--
-- Name: sales_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.sales_id_seq OWNED BY public.sales.id;


--
-- Name: services; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.services (
    id bigint NOT NULL,
    name text NOT NULL,
    description text,
    price double precision NOT NULL,
    updatedate date NOT NULL,
    isactive boolean NOT NULL,
    categoryid bigint NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.services OWNER TO postgres;

--
-- Name: TABLE services; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.services IS 'Услуги';


--
-- Name: COLUMN services.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.id IS 'Первичный ключ';


--
-- Name: COLUMN services.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.name IS 'Название услуги';


--
-- Name: COLUMN services.price; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.price IS 'Цена услуги';


--
-- Name: COLUMN services.updatedate; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.updatedate IS 'Дата последнего изменения';


--
-- Name: COLUMN services.isactive; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.isactive IS 'Активна или нет';


--
-- Name: COLUMN services.categoryid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.categoryid IS 'ID категории';


--
-- Name: COLUMN services.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.services.isdeleted IS 'Флаг удаления';


--
-- Name: services_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.services_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.services_id_seq OWNER TO postgres;

--
-- Name: services_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.services_id_seq OWNED BY public.services.id;


--
-- Name: users; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.users (
    id bigint NOT NULL,
    name text NOT NULL,
    password text NOT NULL,
    roleid bigint NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.users OWNER TO postgres;

--
-- Name: TABLE users; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.users IS 'Пользователи';


--
-- Name: COLUMN users.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.users.id IS 'Первичный ключ';


--
-- Name: COLUMN users.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.users.name IS 'Имя пользователя';


--
-- Name: COLUMN users.password; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.users.password IS 'Пароль';


--
-- Name: COLUMN users.roleid; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.users.roleid IS 'ID роли';


--
-- Name: COLUMN users.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.users.isdeleted IS 'Флаг удаления';


--
-- Name: users_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.users_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.users_id_seq OWNER TO postgres;

--
-- Name: users_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.users_id_seq OWNED BY public.users.id;


--
-- Name: vatrates; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.vatrates (
    id bigint NOT NULL,
    name text NOT NULL,
    rate double precision NOT NULL,
    isdeleted boolean
);


ALTER TABLE public.vatrates OWNER TO postgres;

--
-- Name: TABLE vatrates; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE public.vatrates IS 'Ставки НДС';


--
-- Name: COLUMN vatrates.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.vatrates.id IS 'Первичный ключ';


--
-- Name: COLUMN vatrates.name; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.vatrates.name IS 'Название';


--
-- Name: COLUMN vatrates.isdeleted; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN public.vatrates.isdeleted IS 'Флаг удаления';


--
-- Name: vatrates_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.vatrates_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.vatrates_id_seq OWNER TO postgres;

--
-- Name: vatrates_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.vatrates_id_seq OWNED BY public.vatrates.id;


--
-- Name: barcodes id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.barcodes ALTER COLUMN id SET DEFAULT nextval('public.barcodes_id_seq'::regclass);


--
-- Name: batches id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.batches ALTER COLUMN id SET DEFAULT nextval('public.batches_id_seq'::regclass);


--
-- Name: checks id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.checks ALTER COLUMN id SET DEFAULT nextval('public.checks_id_seq'::regclass);


--
-- Name: employees id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.employees ALTER COLUMN id SET DEFAULT nextval('public.employees_id_seq'::regclass);


--
-- Name: goodcats id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goodcats ALTER COLUMN id SET DEFAULT nextval('public.goodcats_id_seq'::regclass);


--
-- Name: goods id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goods ALTER COLUMN id SET DEFAULT nextval('public.goods_id_seq'::regclass);


--
-- Name: items id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.items ALTER COLUMN id SET DEFAULT nextval('public.items_id_seq'::regclass);


--
-- Name: positions id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.positions ALTER COLUMN id SET DEFAULT nextval('public.positions_id_seq'::regclass);


--
-- Name: roles id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.roles ALTER COLUMN id SET DEFAULT nextval('public.roles_id_seq'::regclass);


--
-- Name: sales id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sales ALTER COLUMN id SET DEFAULT nextval('public.sales_id_seq'::regclass);


--
-- Name: services id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.services ALTER COLUMN id SET DEFAULT nextval('public.services_id_seq'::regclass);


--
-- Name: users id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.users ALTER COLUMN id SET DEFAULT nextval('public.users_id_seq'::regclass);


--
-- Name: vatrates id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.vatrates ALTER COLUMN id SET DEFAULT nextval('public.vatrates_id_seq'::regclass);


--
-- Data for Name: barcodes; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.barcodes (id, barcode, batchid, updatedate, isdeleted) FROM stdin;
\.


--
-- Data for Name: batches; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.batches (id, goodid, qnt, batchnumber, proddate, expdate, writtenoff, updatedate, emoloyeeid, isdeleted, price) FROM stdin;
\.


--
-- Data for Name: checks; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.checks (id, date, "time", totalamount, discountamount, employeeid, isdeleted) FROM stdin;
\.


--
-- Data for Name: discounts; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.discounts (id, name, description, percent, sum, isdeleted) FROM stdin;
\.


--
-- Data for Name: employees; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.employees (id, lastname, firstname, middlename, positionid, userid, updatedate, isdeleted) FROM stdin;
\.


--
-- Data for Name: goodcats; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.goodcats (id, name, description, updatedate, isdeleted) FROM stdin;
\.


--
-- Data for Name: goods; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.goods (id, name, description, categoryid, imageurl, isactive, discountid, updatedate, employeeid, isdeleted) FROM stdin;
\.


--
-- Data for Name: items; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.items (id, batchid, serviceid, isdeleted) FROM stdin;
\.


--
-- Data for Name: positions; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.positions (id, name, isactive, updatedate, isdeleted) FROM stdin;
\.


--
-- Data for Name: roles; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.roles (id, name, level, isblocked, isdeleted) FROM stdin;
1	admin	0	\N	\N
\.


--
-- Data for Name: sales; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.sales (id, itemid, qnt, vatrateid, sum, comment, checkid, isdeleted) FROM stdin;
\.


--
-- Data for Name: services; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.services (id, name, description, price, updatedate, isactive, categoryid, isdeleted) FROM stdin;
\.


--
-- Data for Name: users; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.users (id, name, password, roleid, isdeleted) FROM stdin;
1	root	123456	1	\N
\.


--
-- Data for Name: vatrates; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.vatrates (id, name, rate, isdeleted) FROM stdin;
\.


--
-- Name: barcodes_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.barcodes_id_seq', 1, false);


--
-- Name: batches_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.batches_id_seq', 1, false);


--
-- Name: checks_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.checks_id_seq', 1, false);


--
-- Name: employees_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.employees_id_seq', 1, false);


--
-- Name: goodcats_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.goodcats_id_seq', 1, false);


--
-- Name: goods_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.goods_id_seq', 1, false);


--
-- Name: items_id_s; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.items_id_s', 1, false);


--
-- Name: items_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.items_id_seq', 1, false);


--
-- Name: positions_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.positions_id_seq', 1, false);


--
-- Name: roles_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.roles_id_seq', 1, true);


--
-- Name: sales_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.sales_id_seq', 1, false);


--
-- Name: services_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.services_id_seq', 1, false);


--
-- Name: users_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.users_id_seq', 1, true);


--
-- Name: vatrates_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.vatrates_id_seq', 1, false);


--
-- Name: barcodes barcodes_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.barcodes
    ADD CONSTRAINT barcodes_pk PRIMARY KEY (id);


--
-- Name: batches batches_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.batches
    ADD CONSTRAINT batches_pk PRIMARY KEY (id);


--
-- Name: checks checks_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.checks
    ADD CONSTRAINT checks_pk PRIMARY KEY (id);


--
-- Name: discounts discounts_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.discounts
    ADD CONSTRAINT discounts_pk PRIMARY KEY (id);


--
-- Name: employees employees_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.employees
    ADD CONSTRAINT employees_pk PRIMARY KEY (id);


--
-- Name: goodcats goodcats_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goodcats
    ADD CONSTRAINT goodcats_pk PRIMARY KEY (id);


--
-- Name: goods goods_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goods
    ADD CONSTRAINT goods_pk PRIMARY KEY (id);


--
-- Name: items items_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.items
    ADD CONSTRAINT items_pk PRIMARY KEY (id);


--
-- Name: positions positions_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.positions
    ADD CONSTRAINT positions_pk PRIMARY KEY (id);


--
-- Name: roles roles_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.roles
    ADD CONSTRAINT roles_pk PRIMARY KEY (id);


--
-- Name: sales sales_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sales
    ADD CONSTRAINT sales_pk PRIMARY KEY (id);


--
-- Name: services services_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.services
    ADD CONSTRAINT services_pk PRIMARY KEY (id);


--
-- Name: users users_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.users
    ADD CONSTRAINT users_pk PRIMARY KEY (id);


--
-- Name: vatrates vatrates_pk; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.vatrates
    ADD CONSTRAINT vatrates_pk PRIMARY KEY (id);


--
-- Name: barcodes barcodes_batches_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.barcodes
    ADD CONSTRAINT barcodes_batches_id_fk FOREIGN KEY (batchid) REFERENCES public.batches(id);


--
-- Name: batches batches_employees_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.batches
    ADD CONSTRAINT batches_employees_id_fk FOREIGN KEY (emoloyeeid) REFERENCES public.employees(id);


--
-- Name: batches batches_goods_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.batches
    ADD CONSTRAINT batches_goods_id_fk FOREIGN KEY (goodid) REFERENCES public.goods(id);


--
-- Name: checks checks_employees_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.checks
    ADD CONSTRAINT checks_employees_id_fk FOREIGN KEY (employeeid) REFERENCES public.employees(id);


--
-- Name: employees employees_positions_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.employees
    ADD CONSTRAINT employees_positions_id_fk FOREIGN KEY (positionid) REFERENCES public.positions(id);


--
-- Name: employees employees_users_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.employees
    ADD CONSTRAINT employees_users_id_fk FOREIGN KEY (userid) REFERENCES public.users(id);


--
-- Name: goods goods_discounts_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goods
    ADD CONSTRAINT goods_discounts_id_fk FOREIGN KEY (discountid) REFERENCES public.discounts(id);


--
-- Name: goods goods_employees_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goods
    ADD CONSTRAINT goods_employees_id_fk FOREIGN KEY (employeeid) REFERENCES public.employees(id);


--
-- Name: goods goods_goodcats_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.goods
    ADD CONSTRAINT goods_goodcats_id_fk FOREIGN KEY (categoryid) REFERENCES public.goodcats(id);


--
-- Name: items items_batches_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.items
    ADD CONSTRAINT items_batches_id_fk FOREIGN KEY (batchid) REFERENCES public.batches(id);


--
-- Name: items items_services_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.items
    ADD CONSTRAINT items_services_id_fk FOREIGN KEY (serviceid) REFERENCES public.services(id);


--
-- Name: sales sales_checks_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sales
    ADD CONSTRAINT sales_checks_id_fk FOREIGN KEY (checkid) REFERENCES public.checks(id);


--
-- Name: sales sales_items_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sales
    ADD CONSTRAINT sales_items_id_fk FOREIGN KEY (itemid) REFERENCES public.items(id);


--
-- Name: sales sales_vatrates_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.sales
    ADD CONSTRAINT sales_vatrates_id_fk FOREIGN KEY (vatrateid) REFERENCES public.vatrates(id);


--
-- Name: services services_goodcats_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.services
    ADD CONSTRAINT services_goodcats_id_fk FOREIGN KEY (categoryid) REFERENCES public.goodcats(id);


--
-- Name: users users_roles_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.users
    ADD CONSTRAINT users_roles_id_fk FOREIGN KEY (roleid) REFERENCES public.roles(id);


--
-- PostgreSQL database dump complete
--

\unrestrict edvoeSmz5ApTYwvGHut85GfDRaf0K876FIzdm9IdHk4bcEywW3AMMrArboJTTir

