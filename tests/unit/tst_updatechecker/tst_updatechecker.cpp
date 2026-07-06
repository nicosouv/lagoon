#include <QtTest>

#include "updatechecker.h"

class TestUpdateChecker : public QObject
{
    Q_OBJECT

private slots:
    void versionComparison_data();
    void versionComparison();
    void currentVersionComesFromBuild();
};

void TestUpdateChecker::versionComparison_data()
{
    QTest::addColumn<QString>("latest");
    QTest::addColumn<QString>("current");
    QTest::addColumn<bool>("isNewer");

    QTest::newRow("equal") << "1.2.3" << "1.2.3" << false;
    QTest::newRow("patch newer") << "1.2.4" << "1.2.3" << true;
    QTest::newRow("patch older") << "1.2.2" << "1.2.3" << false;
    QTest::newRow("minor newer") << "1.3.0" << "1.2.9" << true;
    QTest::newRow("minor older") << "1.1.9" << "1.2.0" << false;
    QTest::newRow("major newer") << "2.0.0" << "1.9.9" << true;
    QTest::newRow("major older") << "1.9.9" << "2.0.0" << false;
    QTest::newRow("short latest") << "1.3" << "1.2.9" << true;
    QTest::newRow("short current") << "1.2.1" << "1.2" << true;
    QTest::newRow("both short equal") << "1.2" << "1.2" << false;
    QTest::newRow("double digit parts") << "0.38.10" << "0.38.9" << true;
}

void TestUpdateChecker::versionComparison()
{
    QFETCH(QString, latest);
    QFETCH(QString, current);
    QFETCH(bool, isNewer);

    QCOMPARE(UpdateChecker::isNewerVersion(latest, current), isNewer);
}

void TestUpdateChecker::currentVersionComesFromBuild()
{
    UpdateChecker checker;
    // APP_VERSION is injected by qmake from the spec file; the fallback
    // "0.0.0" would make every release look like an update
    QVERIFY(!checker.currentVersion().isEmpty());
}

QTEST_GUILESS_MAIN(TestUpdateChecker)
#include "tst_updatechecker.moc"
