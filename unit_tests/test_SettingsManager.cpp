#include "test_SettingsManager.h"
#include <QCoreApplication>
#include <QDebug>

TestSettingsManager::TestSettingsManager()
{
}

TestSettingsManager::~TestSettingsManager()
{
}

void TestSettingsManager::initTestCase()
{
    qDebug() << "Инициализация тестового набора для SettingsManager";
    QCoreApplication::setOrganizationName("easyPOSTest");
    QCoreApplication::setApplicationName("unit_tests");
}

void TestSettingsManager::cleanupTestCase()
{
    qDebug() << "Завершение тестового набора для SettingsManager";
}

void TestSettingsManager::init()
{
}

void TestSettingsManager::cleanup()
{
    SettingsManager sm;
    sm.remove("test/key1");
    sm.remove("test/key2");
    sm.remove("test/key3");
    sm.remove("test/intkey");
    sm.remove("test/boolkey");
    sm.sync();
}

void TestSettingsManager::testConstructor()
{
    SettingsManager sm;
    QVERIFY(sm.value("nonexistent").isNull());
}

void TestSettingsManager::testValueSetValue()
{
    SettingsManager sm;
    sm.setValue("test/key1", "value1");
    QCOMPARE(sm.value("test/key1").toString(), QString("value1"));
    QCOMPARE(sm.value("test/key1", "default").toString(), QString("value1"));

    sm.setValue("test/key2", 42);
    QCOMPARE(sm.value("test/key2").toInt(), 42);

    QVariant def = sm.value("test/nonexistent", "defaultVal");
    QCOMPARE(def.toString(), QString("defaultVal"));
}

void TestSettingsManager::testContains()
{
    SettingsManager sm;
    QVERIFY(!sm.contains("test/key3"));
    sm.setValue("test/key3", "x");
    QVERIFY(sm.contains("test/key3"));
    sm.remove("test/key3");
    QVERIFY(!sm.contains("test/key3"));
}

void TestSettingsManager::testRemove()
{
    SettingsManager sm;
    sm.setValue("test/key1", "v");
    QVERIFY(sm.contains("test/key1"));
    sm.remove("test/key1");
    QVERIFY(!sm.contains("test/key1"));
    QVERIFY(sm.value("test/key1").isNull());
}

void TestSettingsManager::testStringValue()
{
    SettingsManager sm;
    QCOMPARE(sm.stringValue("test/key1"), QString());
    QCOMPARE(sm.stringValue("test/key1", "def"), QString("def"));
    sm.setValue("test/key1", "hello");
    QCOMPARE(sm.stringValue("test/key1"), QString("hello"));
    QCOMPARE(sm.stringValue("test/key1", "ignored"), QString("hello"));
}

void TestSettingsManager::testIntValue()
{
    SettingsManager sm;
    QCOMPARE(sm.intValue("test/intkey"), 0);
    QCOMPARE(sm.intValue("test/intkey", 99), 99);
    sm.setValue("test/intkey", 123);
    QCOMPARE(sm.intValue("test/intkey"), 123);
    QCOMPARE(sm.intValue("test/intkey", 0), 123);
}

void TestSettingsManager::testBoolValue()
{
    SettingsManager sm;
    QVERIFY(!sm.boolValue("test/boolkey"));
    QVERIFY(sm.boolValue("test/boolkey", true));
    sm.setValue("test/boolkey", true);
    QVERIFY(sm.boolValue("test/boolkey"));
    sm.setValue("test/boolkey", false);
    QVERIFY(!sm.boolValue("test/boolkey"));
}

void TestSettingsManager::testLoadSaveDatabaseSettings()
{
    SettingsManager sm;

    PostgreSQLAuth auth;
    auth.host = "testhost";
    auth.port = 5433;
    auth.database = "testdb";
    auth.username = "testuser";
    auth.password = "secret";
    auth.sslMode = "require";
    auth.connectTimeout = 5;

    sm.saveDatabaseSettings(auth);
    PostgreSQLAuth loaded = sm.loadDatabaseSettings();

    QCOMPARE(loaded.host, auth.host);
    QCOMPARE(loaded.port, auth.port);
    QCOMPARE(loaded.database, auth.database);
    QCOMPARE(loaded.username, auth.username);
    QCOMPARE(loaded.password, auth.password);
    QCOMPARE(loaded.sslMode, auth.sslMode);
    QCOMPARE(loaded.connectTimeout, auth.connectTimeout);
}
