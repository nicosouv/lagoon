#include <QtTest>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>

#include "models/conversationmodel.h"
#include "models/usermodel.h"

class TestConversationModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void parseChannel();
    void parseIm();
    void unreadFromLastRead();
    void unreadCountDisplayWins();
    void nullLatestGivesNoTimestamp();
    void sortOrdering();
    void sectionMapping();
    void markAsReadMovesSingleRow();
    void incrementUnreadMovesSingleRow();
    void incrementUnreadIgnoresOldMessages();
    void toggleStarMovesSingleRow();
    void batchUnreadDoesNotSortUntilResort();
    void dmNameResolution();

private:
    QJsonObject channelJson(const QString &id, const QString &name,
                            bool isPrivate = false, bool isMember = true) const;
    QJsonObject imJson(const QString &id, const QString &userId) const;
    QJsonObject mpimJson(const QString &id, const QString &name) const;
    QString nameAt(const ConversationModel &model, int row) const;
    QString idAt(const ConversationModel &model, int row) const;
};

QJsonObject TestConversationModel::channelJson(const QString &id, const QString &name,
                                               bool isPrivate, bool isMember) const
{
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["is_channel"] = true;
    json["is_private"] = isPrivate;
    json["is_member"] = isMember;
    return json;
}

QJsonObject TestConversationModel::imJson(const QString &id, const QString &userId) const
{
    QJsonObject json;
    json["id"] = id;
    json["is_im"] = true;
    json["user"] = userId;
    return json;
}

QJsonObject TestConversationModel::mpimJson(const QString &id, const QString &name) const
{
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["is_mpim"] = true;
    return json;
}

QString TestConversationModel::nameAt(const ConversationModel &model, int row) const
{
    return model.data(model.index(row, 0), ConversationModel::NameRole).toString();
}

QString TestConversationModel::idAt(const ConversationModel &model, int row) const
{
    return model.data(model.index(row, 0), ConversationModel::IdRole).toString();
}

void TestConversationModel::initTestCase()
{
    // Keep settings written by the models out of the real user configuration
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_conversationmodel"));
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_conversationmodel"));
}

void TestConversationModel::init()
{
    // Wipe persisted state so tests are independent
    QSettings("harbour-lagoon", "starred-channels").clear();
    QSettings("harbour-lagoon", "last-read-timestamps").clear();
    QSettings("harbour-lagoon", "users").clear();
    QSettings("harbour-lagoon", "users-full").clear();
}

void TestConversationModel::parseChannel()
{
    ConversationModel model;

    QJsonObject json = channelJson("C1", "general", true);
    QJsonObject topic;
    topic["value"] = "the topic";
    json["topic"] = topic;
    QJsonObject purpose;
    purpose["value"] = "the purpose";
    json["purpose"] = purpose;
    QJsonObject latest;
    latest["ts"] = "1700000000.000100";
    json["latest"] = latest;

    QJsonArray array;
    array.append(json);
    model.updateConversations(array);

    QCOMPARE(model.rowCount(), 1);
    QVariantMap conv = model.get(0);
    QCOMPARE(conv["id"].toString(), QString("C1"));
    QCOMPARE(conv["name"].toString(), QString("general"));
    QCOMPARE(conv["type"].toString(), QString("channel"));
    QCOMPARE(conv["isPrivate"].toBool(), true);
    QCOMPARE(conv["isMember"].toBool(), true);
    QCOMPARE(conv["topic"].toString(), QString("the topic"));
    QCOMPARE(conv["purpose"].toString(), QString("the purpose"));
    QCOMPARE(conv["unreadCount"].toInt(), 0);
    QCOMPARE(conv["lastMessageTime"].toLongLong(), Q_INT64_C(1700000000000));
}

void TestConversationModel::parseIm()
{
    ConversationModel model;

    QJsonArray array;
    array.append(imJson("D1", "U42"));
    model.updateConversations(array);

    QCOMPARE(model.rowCount(), 1);
    QVariantMap conv = model.get(0);
    QCOMPARE(conv["type"].toString(), QString("im"));
    QCOMPARE(conv["userId"].toString(), QString("U42"));
}

void TestConversationModel::unreadFromLastRead()
{
    ConversationModel model;

    // last_read older than latest -> at least one unread
    QJsonObject unreadChannel = channelJson("C1", "unread-chan");
    unreadChannel["last_read"] = "1700000000.000000";
    QJsonObject latest;
    latest["ts"] = "1700000100.000000";
    unreadChannel["latest"] = latest;

    // last_read equal to latest -> read
    QJsonObject readChannel = channelJson("C2", "read-chan");
    readChannel["last_read"] = "1700000100.000000";
    readChannel["latest"] = latest;

    QJsonArray array;
    array.append(unreadChannel);
    array.append(readChannel);
    model.updateConversations(array);

    QCOMPARE(model.get(model.rowCount() - 1)["name"].toString(), QString("read-chan"));
    // Unread channel sorts first
    QVariantMap first = model.get(0);
    QCOMPARE(first["name"].toString(), QString("unread-chan"));
    QCOMPARE(first["unreadCount"].toInt(), 1);
    QCOMPARE(model.get(1)["unreadCount"].toInt(), 0);
}

void TestConversationModel::unreadCountDisplayWins()
{
    ConversationModel model;

    QJsonObject im = imJson("D1", "U1");
    im["unread_count"] = 5;
    im["unread_count_display"] = 2;

    QJsonArray array;
    array.append(im);
    model.updateConversations(array);

    QCOMPARE(model.get(0)["unreadCount"].toInt(), 2);
}

void TestConversationModel::nullLatestGivesNoTimestamp()
{
    ConversationModel model;

    QJsonObject json = channelJson("C1", "empty-chan");
    json["latest"] = QJsonValue::Null;

    QJsonArray array;
    array.append(json);
    model.updateConversations(array);

    QCOMPARE(model.get(0)["lastMessageTime"].toLongLong(), Q_INT64_C(0));
    QCOMPARE(model.get(0)["unreadCount"].toInt(), 0);
}

void TestConversationModel::sortOrdering()
{
    ConversationModel model;

    QJsonObject starredIm = imJson("D1", "U1");
    starredIm["is_starred"] = true;

    QJsonObject unreadChannel = channelJson("C1", "zulu");
    unreadChannel["unread_count"] = 3;

    QJsonArray array;
    array.append(imJson("D2", "U2"));                 // im, read
    array.append(channelJson("C2", "beta"));          // channel, read
    array.append(unreadChannel);                      // channel, unread
    array.append(mpimJson("G1", "group-chat"));       // mpim, read
    array.append(channelJson("C3", "alpha"));         // channel, read
    array.append(starredIm);                          // starred im
    model.updateConversations(array);

    // starred > unread > channels (alpha) > mpim > im
    QCOMPARE(idAt(model, 0), QString("D1"));
    QCOMPARE(idAt(model, 1), QString("C1"));
    QCOMPARE(nameAt(model, 2), QString("alpha"));
    QCOMPARE(nameAt(model, 3), QString("beta"));
    QCOMPARE(idAt(model, 4), QString("G1"));
    QCOMPARE(idAt(model, 5), QString("D2"));
}

void TestConversationModel::sectionMapping()
{
    ConversationModel model;

    QJsonObject starred = channelJson("C1", "starred-chan");
    starred["is_starred"] = true;
    QJsonObject unread = channelJson("C2", "unread-chan");
    unread["unread_count"] = 1;

    QJsonArray array;
    array.append(starred);
    array.append(unread);
    array.append(channelJson("C3", "plain-chan"));
    array.append(imJson("D1", "U1"));
    array.append(mpimJson("G1", "group"));
    model.updateConversations(array);

    QCOMPARE(model.get(0)["section"].toString(), QString("starred"));
    QCOMPARE(model.get(1)["section"].toString(), QString("unread"));
    QCOMPARE(model.get(2)["section"].toString(), QString("channel"));
    QCOMPARE(model.get(3)["section"].toString(), QString("mpim"));
    QCOMPARE(model.get(4)["section"].toString(), QString("im"));
}

void TestConversationModel::markAsReadMovesSingleRow()
{
    ConversationModel model;
    model.setTeamId("TTEST");

    QJsonObject unread = channelJson("C1", "zulu");
    unread["unread_count"] = 2;

    QJsonArray array;
    array.append(unread);
    array.append(channelJson("C2", "alpha"));
    array.append(channelJson("C3", "beta"));
    model.updateConversations(array);
    QCOMPARE(idAt(model, 0), QString("C1"));  // unread first

    QSignalSpy movedSpy(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.markAsRead("C1", Q_INT64_C(1700000200000));

    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(movedSpy.count(), 1);
    // zulu is read again and sorts alphabetically last
    QCOMPARE(nameAt(model, 0), QString("alpha"));
    QCOMPARE(nameAt(model, 1), QString("beta"));
    QCOMPARE(nameAt(model, 2), QString("zulu"));
    QCOMPARE(model.get(2)["unreadCount"].toInt(), 0);
}

void TestConversationModel::incrementUnreadMovesSingleRow()
{
    ConversationModel model;
    model.setTeamId("TTEST");

    QJsonArray array;
    array.append(channelJson("C1", "alpha"));
    array.append(channelJson("C2", "beta"));
    model.updateConversations(array);
    QCOMPARE(idAt(model, 0), QString("C1"));

    QSignalSpy movedSpy(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.incrementUnread("C2", Q_INT64_C(1700000300000));

    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(movedSpy.count(), 1);
    QCOMPARE(idAt(model, 0), QString("C2"));
    QCOMPARE(model.get(0)["unreadCount"].toInt(), 1);

    // A second message increments without another section change
    model.incrementUnread("C2", Q_INT64_C(1700000400000));
    QCOMPARE(model.get(0)["unreadCount"].toInt(), 2);
}

void TestConversationModel::incrementUnreadIgnoresOldMessages()
{
    ConversationModel model;
    model.setTeamId("TTEST");

    QJsonArray array;
    array.append(channelJson("C1", "alpha"));
    model.updateConversations(array);

    // Mark as read at t=1700000500000; an older RTM message must not re-flag
    model.markAsRead("C1", Q_INT64_C(1700000500000));
    model.incrementUnread("C1", Q_INT64_C(1700000400000));

    QCOMPARE(model.get(0)["unreadCount"].toInt(), 0);
}

void TestConversationModel::toggleStarMovesSingleRow()
{
    ConversationModel model;
    model.setTeamId("TTEST");

    QJsonArray array;
    array.append(channelJson("C1", "alpha"));
    array.append(channelJson("C2", "beta"));
    array.append(imJson("D1", "U1"));
    model.updateConversations(array);
    QCOMPARE(idAt(model, 2), QString("D1"));  // im sorts last

    QSignalSpy movedSpy(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    model.toggleStar("D1");
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(movedSpy.count(), 1);
    QCOMPARE(idAt(model, 0), QString("D1"));
    QCOMPARE(model.get(0)["section"].toString(), QString("starred"));

    model.toggleStar("D1");
    QCOMPARE(idAt(model, 2), QString("D1"));
}

void TestConversationModel::batchUnreadDoesNotSortUntilResort()
{
    ConversationModel model;

    QJsonArray array;
    array.append(channelJson("C1", "alpha"));
    array.append(channelJson("C2", "beta"));
    array.append(channelJson("C3", "gamma"));
    model.updateConversations(array);

    QSignalSpy movedSpy(&model, &QAbstractItemModel::rowsMoved);
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy layoutSpy(&model, &QAbstractItemModel::layoutChanged);

    // Batch results arrive: no reordering yet
    model.updateUnreadInfo("C3", 4, Q_INT64_C(1700000100000));
    model.updateUnreadInfo("C2", 1, Q_INT64_C(1700000200000));
    QCOMPARE(movedSpy.count(), 0);
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(layoutSpy.count(), 0);
    QCOMPARE(nameAt(model, 0), QString("alpha"));

    // allUnreadsFetched fires -> one layout change, list sorted
    model.resortAndNotify();
    QCOMPARE(layoutSpy.count(), 1);
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(nameAt(model, 0), QString("gamma"));  // 4 unread
    QCOMPARE(nameAt(model, 1), QString("beta"));   // 1 unread
    QCOMPARE(nameAt(model, 2), QString("alpha"));
}

void TestConversationModel::dmNameResolution()
{
    ConversationModel model;
    UserModel userModel;
    model.setUserModel(&userModel);

    QJsonArray conversations;
    conversations.append(imJson("D1", "U42"));
    model.updateConversations(conversations);

    // Unknown user: falls back to the user id
    QCOMPARE(nameAt(model, 0), QString("U42"));

    QJsonObject profile;
    profile["display_name"] = "Jane";
    QJsonObject user;
    user["id"] = "U42";
    user["name"] = "jane.doe";
    user["profile"] = profile;
    QJsonArray users;
    users.append(user);
    userModel.updateUsers(users);

    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);
    model.refreshDmNames();

    QCOMPARE(dataSpy.count(), 1);
    QCOMPARE(nameAt(model, 0), QString("Jane"));
}

QTEST_GUILESS_MAIN(TestConversationModel)
#include "tst_conversationmodel.moc"
