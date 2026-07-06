#include <QtTest>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSettings>

#include "workspacemanager.h"

class TestWorkspaceManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void addWorkspaceDedupesByTeamId();
    void removeDuplicatesKeepsMostRecent();
    void currentIndexStableAcrossRemove();
    void settingsFileIsOwnerOnly();

private:
    void seedLegacyWorkspace(QSettings &settings, int index, const QString &name,
                             const QString &teamId, const QString &token,
                             const QDateTime &lastUsed, bool isActive = false);
};

void TestWorkspaceManager::initTestCase()
{
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_workspacemanager"));
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_workspacemanager"));
}

void TestWorkspaceManager::init()
{
    QSettings("harbour-lagoon", "workspaces").clear();
}

void TestWorkspaceManager::seedLegacyWorkspace(QSettings &settings, int index,
                                               const QString &name, const QString &teamId,
                                               const QString &token, const QDateTime &lastUsed,
                                               bool isActive)
{
    settings.setArrayIndex(index);
    settings.setValue("id", QString("id-%1").arg(index));
    settings.setValue("name", name);
    settings.setValue("token", token);
    settings.setValue("teamId", teamId);
    settings.setValue("userId", "U1");
    settings.setValue("domain", name + ".slack.com");
    settings.setValue("isActive", isActive);
    settings.setValue("lastUsed", lastUsed);
}

void TestWorkspaceManager::addWorkspaceDedupesByTeamId()
{
    WorkspaceManager manager;

    manager.addWorkspace("Team", "xoxp-old", "T1", "U1", "team.slack.com");
    manager.addWorkspace("Team Renamed", "xoxp-new", "T1", "U1", "team.slack.com");

    QCOMPARE(manager.workspaceCount(), 1);
    QModelIndex idx = manager.index(0, 0);
    QCOMPARE(manager.data(idx, WorkspaceManager::NameRole).toString(), QString("Team Renamed"));
    QCOMPARE(manager.data(idx, WorkspaceManager::TokenRole).toString(), QString("xoxp-new"));
}

void TestWorkspaceManager::removeDuplicatesKeepsMostRecent()
{
    // Seed the settings with duplicates (as older releases could create)
    {
        QSettings settings("harbour-lagoon", "workspaces");
        settings.beginWriteArray("workspaces");
        seedLegacyWorkspace(settings, 0, "Team", "T1", "xoxp-old",
                            QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1600000000000)));
        seedLegacyWorkspace(settings, 1, "Team", "T1", "xoxp-recent",
                            QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1700000000000)), true);
        seedLegacyWorkspace(settings, 2, "", "", "xoxp-broken",
                            QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1700000000000)));
        settings.endArray();
        settings.sync();
    }

    // Construction loads and deduplicates
    WorkspaceManager manager;

    QCOMPARE(manager.workspaceCount(), 1);
    QCOMPARE(manager.data(manager.index(0, 0), WorkspaceManager::TokenRole).toString(),
             QString("xoxp-recent"));
}

void TestWorkspaceManager::currentIndexStableAcrossRemove()
{
    WorkspaceManager manager;
    manager.addWorkspace("Alpha", "xoxp-a", "T1", "U1", "a.slack.com");
    manager.addWorkspace("Beta", "xoxp-b", "T2", "U1", "b.slack.com");
    manager.addWorkspace("Gamma", "xoxp-c", "T3", "U1", "c.slack.com");

    manager.setCurrentWorkspaceIndex(1);
    QCOMPARE(manager.currentWorkspaceName(), QString("Beta"));

    // Removing a workspace before the current one keeps the selection
    manager.removeWorkspace(0);
    QCOMPARE(manager.currentWorkspaceName(), QString("Beta"));
    QCOMPARE(manager.currentWorkspaceIndex(), 0);

    // Removing the current one falls back to the first remaining
    manager.removeWorkspace(0);
    QCOMPARE(manager.workspaceCount(), 1);
    QCOMPARE(manager.currentWorkspaceName(), QString("Gamma"));
}

void TestWorkspaceManager::settingsFileIsOwnerOnly()
{
    WorkspaceManager manager;
    manager.addWorkspace("Team", "xoxp-secret", "T1", "U1", "team.slack.com");

    QSettings settings("harbour-lagoon", "workspaces");
    QFile::Permissions permissions = QFile(settings.fileName()).permissions();

    QVERIFY(permissions.testFlag(QFile::ReadOwner));
    QVERIFY(!permissions.testFlag(QFile::ReadGroup));
    QVERIFY(!permissions.testFlag(QFile::ReadOther));
}

QTEST_GUILESS_MAIN(TestWorkspaceManager)
#include "tst_workspacemanager.moc"
