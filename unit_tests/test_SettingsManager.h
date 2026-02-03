#ifndef TEST_SETTINGSMANAGER_H
#define TEST_SETTINGSMANAGER_H

#include <QtTest>
#include <QObject>
#include "../settings/settingsmanager.h"
#include "../db/structures.h"

class TestSettingsManager : public QObject
{
    Q_OBJECT

public:
    TestSettingsManager();
    ~TestSettingsManager();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstructor();
    void testValueSetValue();
    void testContains();
    void testRemove();
    void testStringValue();
    void testIntValue();
    void testBoolValue();
    void testLoadSaveDatabaseSettings();
};

#endif // TEST_SETTINGSMANAGER_H
