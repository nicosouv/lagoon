#include <QtTest>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>

#include "notificationcoordinator.h"
#include "models/conversationmodel.h"
#include "models/usermodel.h"

class TestNotificationCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void notifiesForOtherUsersMessage();
    void ownMessagesAreSilent();
    void activeChannelIsSilent();
    void mentionDetection_data();
    void mentionDetection();
    void mentionTriggersMentionNotification();
    void dmNameIsResolvedInNotification();

private:
    QJsonObject rtmMessage(const QString &channelId, const QString &userId,
                           const QString &text) const;
};

QJsonObject TestNotificationCoordinator::rtmMessage(const QString &channelId,
                                                    const QString &userId,
                                                    const QString &text) const
{
    QJsonObject message;
    message["type"] = "message";
    message["channel"] = channelId;
    message["user"] = userId;
    message["text"] = text;
    message["ts"] = "1700000000.000100";
    return message;
}

void TestNotificationCoordinator::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_notificationcoordinator"));
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       QDir::temp().filePath("tst_notificationcoordinator"));
}

void TestNotificationCoordinator::init()
{
    QSettings("harbour-lagoon", "starred-channels").clear();
    QSettings("harbour-lagoon", "last-read-timestamps").clear();
    QSettings("harbour-lagoon", "users").clear();
}

void TestNotificationCoordinator::notifiesForOtherUsersMessage()
{
    NotificationCoordinator coordinator;
    coordinator.setCurrentUserId("U_ME");

    QSignalSpy messageSpy(&coordinator, &NotificationCoordinator::messageNotification);
    coordinator.handleRtmMessage(rtmMessage("C1", "U_OTHER", "hello"));

    QCOMPARE(messageSpy.count(), 1);
    QCOMPARE(messageSpy.first().at(1).toString(), QString("U_OTHER"));  // name falls back to id
    QCOMPARE(messageSpy.first().at(3).toString(), QString("C1"));
}

void TestNotificationCoordinator::ownMessagesAreSilent()
{
    NotificationCoordinator coordinator;
    coordinator.setCurrentUserId("U_ME");

    QSignalSpy messageSpy(&coordinator, &NotificationCoordinator::messageNotification);
    QSignalSpy mentionSpy(&coordinator, &NotificationCoordinator::mentionNotification);

    // RTM echoes our own sent message
    coordinator.handleRtmMessage(rtmMessage("C1", "U_ME", "my own message"));

    QCOMPARE(messageSpy.count(), 0);
    QCOMPARE(mentionSpy.count(), 0);
}

void TestNotificationCoordinator::activeChannelIsSilent()
{
    NotificationCoordinator coordinator;
    coordinator.setCurrentUserId("U_ME");
    coordinator.setActiveChannelId("C_OPEN");

    QSignalSpy messageSpy(&coordinator, &NotificationCoordinator::messageNotification);

    // Message in the conversation currently on screen: no notification
    coordinator.handleRtmMessage(rtmMessage("C_OPEN", "U_OTHER", "hi"));
    QCOMPARE(messageSpy.count(), 0);

    // Message elsewhere still notifies
    coordinator.handleRtmMessage(rtmMessage("C_ELSEWHERE", "U_OTHER", "hi"));
    QCOMPARE(messageSpy.count(), 1);

    // Leaving the conversation re-enables notifications for it
    coordinator.setActiveChannelId(QString());
    coordinator.handleRtmMessage(rtmMessage("C_OPEN", "U_OTHER", "hi again"));
    QCOMPARE(messageSpy.count(), 2);
}

void TestNotificationCoordinator::mentionDetection_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("mention");

    QTest::newRow("real mention") << "hey <@U_ME> look" << true;
    QTest::newRow("other user mentioned") << "hey <@U_OTHER> look" << false;
    QTest::newRow("plain at sign") << "mail me at nico@example.org" << false;
    QTest::newRow("at word") << "see @channel notes" << false;
    QTest::newRow("no at") << "plain text" << false;
}

void TestNotificationCoordinator::mentionDetection()
{
    QFETCH(QString, text);
    QFETCH(bool, mention);

    QCOMPARE(NotificationCoordinator::isMention(text, "U_ME"), mention);
}

void TestNotificationCoordinator::mentionTriggersMentionNotification()
{
    NotificationCoordinator coordinator;
    coordinator.setCurrentUserId("U_ME");

    QSignalSpy messageSpy(&coordinator, &NotificationCoordinator::messageNotification);
    QSignalSpy mentionSpy(&coordinator, &NotificationCoordinator::mentionNotification);

    coordinator.handleRtmMessage(rtmMessage("C1", "U_OTHER", "ping <@U_ME>!"));

    QCOMPARE(mentionSpy.count(), 1);
    QCOMPARE(messageSpy.count(), 0);
}

void TestNotificationCoordinator::dmNameIsResolvedInNotification()
{
    UserModel users;
    QJsonObject profile;
    profile["display_name"] = "Jane";
    QJsonObject user;
    user["id"] = "U42";
    user["name"] = "jane.doe";
    user["profile"] = profile;
    QJsonArray userArray;
    userArray.append(user);
    users.updateUsers(userArray);

    ConversationModel conversations;
    conversations.setUserModel(&users);
    QJsonObject im;
    im["id"] = "D1";
    im["is_im"] = true;
    im["user"] = "U42";
    QJsonArray conversationArray;
    conversationArray.append(im);
    conversations.updateConversations(conversationArray);

    NotificationCoordinator coordinator;
    coordinator.setModels(&conversations, &users);
    coordinator.setCurrentUserId("U_ME");

    QSignalSpy messageSpy(&coordinator, &NotificationCoordinator::messageNotification);
    coordinator.handleRtmMessage(rtmMessage("D1", "U42", "hi"));

    QCOMPARE(messageSpy.count(), 1);
    QCOMPARE(messageSpy.first().at(0).toString(), QString("Jane"));  // channel name = DM peer
    QCOMPARE(messageSpy.first().at(1).toString(), QString("Jane"));  // sender name
}

QTEST_GUILESS_MAIN(TestNotificationCoordinator)
#include "tst_notificationcoordinator.moc"
