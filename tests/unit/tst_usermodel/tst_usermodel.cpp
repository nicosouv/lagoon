#include <QtTest>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

#include "models/usermodel.h"
#include "cache/userdb.h"

class TestUserModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void namePriority();
    void hashLookup();
    void searchUsers();
    void cacheRoundTrip();
    void staleCacheIsNotFresh();
    void legacySettingsCacheIsMigrated();

private:
    QJsonObject userJson(const QString &id, const QString &name,
                         const QString &realName = QString(),
                         const QString &displayName = QString(),
                         bool isBot = false) const;
    QString databasePath() const;
};

QJsonObject TestUserModel::userJson(const QString &id, const QString &name,
                                    const QString &realName,
                                    const QString &displayName, bool isBot) const
{
    QJsonObject profile;
    profile["real_name"] = realName;
    profile["display_name"] = displayName;
    profile["image_72"] = QString("https://avatars.test/%1.png").arg(id);

    QJsonObject user;
    user["id"] = id;
    user["name"] = name;
    user["is_bot"] = isBot;
    user["profile"] = profile;
    return user;
}

QString TestUserModel::databasePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + QStringLiteral("/users.db");
}

void TestUserModel::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_usermodel"));
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_usermodel"));
}

void TestUserModel::init()
{
    QFile::remove(databasePath());
    QSettings("harbour-lagoon", "users").clear();
    QSettings("harbour-lagoon", "users-full").clear();
}

void TestUserModel::namePriority()
{
    UserModel model;

    QJsonArray users;
    users.append(userJson("U1", "uname", "Real Name", "Display Name"));
    users.append(userJson("U2", "uname2", "Real Only"));
    users.append(userJson("U3", "uname3"));
    model.updateUsers(users);

    // displayName > realName > name
    QCOMPARE(model.getUserName("U1"), QString("Display Name"));
    QCOMPARE(model.getUserName("U2"), QString("Real Only"));
    QCOMPARE(model.getUserName("U3"), QString("uname3"));
    // Unknown user falls back to the id
    QCOMPARE(model.getUserName("U_UNKNOWN"), QString("U_UNKNOWN"));
}

void TestUserModel::hashLookup()
{
    UserModel model;

    QJsonArray users;
    for (int i = 0; i < 100; ++i) {
        users.append(userJson(QString("U%1").arg(i), QString("user%1").arg(i)));
    }
    model.updateUsers(users);

    QCOMPARE(model.getUserName("U42"), QString("user42"));
    QCOMPARE(model.getUserAvatar("U99"), QString("https://avatars.test/U99.png"));

    // The index survives additions and clears
    QJsonObject extra = userJson("U100", "user100");
    model.addUser(extra);
    QCOMPARE(model.getUserName("U100"), QString("user100"));

    model.clear();
    QCOMPARE(model.getUserAvatar("U42"), QString());
}

void TestUserModel::searchUsers()
{
    UserModel model;

    QJsonArray users;
    users.append(userJson("U1", "alice.doe", "Alice Doe", "alice"));
    users.append(userJson("U2", "bob", "Bob Martin"));
    users.append(userJson("U3", "alicebot", "Alice Bot", "", true));  // bot: skipped
    model.updateUsers(users);

    QVariantList results = model.searchUsers("alice");
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().toMap()["id"].toString(), QString("U1"));

    // Matches realName too, case-insensitive
    results = model.searchUsers("MARTIN");
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().toMap()["id"].toString(), QString("U2"));

    QCOMPARE(model.searchUsers("").count(), 0);
}

void TestUserModel::cacheRoundTrip()
{
    {
        UserModel writer;
        QJsonArray users;
        users.append(userJson("U1", "uname", "Real Name", "Display Name"));
        users.append(userJson("U2", "bot", "Bot", "", true));
        writer.updateUsers(users, "TTEAM");
    }

    // A fresh model (new process simulation) reads back from SQLite
    UserModel reader;
    QVERIFY(reader.hasFreshCache("TTEAM"));
    QVERIFY(reader.loadUsersFromCache("TTEAM"));
    QCOMPARE(reader.rowCount(), 2);
    QCOMPARE(reader.getUserName("U1"), QString("Display Name"));
    QCOMPARE(reader.getUserAvatar("U1"), QString("https://avatars.test/U1.png"));
    QCOMPARE(reader.userCount(true), 1);  // bot excluded

    // Unknown team: nothing cached
    QVERIFY(!reader.hasFreshCache("T_OTHER"));
    QVERIFY(!reader.loadUsersFromCache("T_OTHER"));
}

void TestUserModel::staleCacheIsNotFresh()
{
    UserDb db;
    QList<UserDb::CachedUser> users;
    UserDb::CachedUser user;
    user.id = "U1";
    user.name = "old";
    user.isBot = false;
    users.append(user);

    // Written 7 hours ago: beyond the 6h validity window
    qint64 sevenHoursAgo = QDateTime::currentMSecsSinceEpoch() - Q_INT64_C(7 * 60 * 60 * 1000);
    QVERIFY(db.saveUsers("TTEAM", users, sevenHoursAgo));

    UserModel model;
    QVERIFY(!model.hasFreshCache("TTEAM"));
    // Data is still loadable even if stale
    QVERIFY(model.loadUsersFromCache("TTEAM"));
}

void TestUserModel::legacySettingsCacheIsMigrated()
{
    // Seed the old INI format
    {
        QSettings legacy("harbour-lagoon", "users-full");
        legacy.setValue("timestamp/TTEAM", QDateTime::currentMSecsSinceEpoch());
        legacy.setValue("count/TTEAM", 1);
        legacy.setValue("users/TTEAM/0/id", "U1");
        legacy.setValue("users/TTEAM/0/name", "legacy-user");
        legacy.setValue("users/TTEAM/0/displayName", "Legacy");
        legacy.sync();
    }

    // Constructing the model migrates the INI cache to SQLite and clears it
    UserModel model;
    QVERIFY(model.hasFreshCache("TTEAM"));
    QVERIFY(model.loadUsersFromCache("TTEAM"));
    QCOMPARE(model.getUserName("U1"), QString("Legacy"));

    QSettings legacy("harbour-lagoon", "users-full");
    QVERIFY(legacy.allKeys().isEmpty());
}

QTEST_GUILESS_MAIN(TestUserModel)
#include "tst_usermodel.moc"
