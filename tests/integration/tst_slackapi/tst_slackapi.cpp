#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>

#include "slackapi.h"
#include "mockslackserver.h"

class TestSlackAPI : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void authTestSuccess();
    void authTestFailure();
    void conversationsSignalPayload();
    void unreadExtractionFromDisplayCount();
    void unreadExtractionNullLatest();
    void batchPacingOfUnreadFetches();
    void okFalseEmitsApiError();
    void postAndGetRouting();
    void unreadResyncFlagLifecycle();

private:
    void authenticate();

    MockSlackServer *m_server;
    SlackAPI *m_api;
};

void TestSlackAPI::init()
{
    m_server = new MockSlackServer;
    QVERIFY(m_server->start());

    m_api = new SlackAPI;
    m_api->setApiBaseUrl(m_server->baseUrl());
    m_api->setAutoRefresh(false);  // keep the polling timer out of the tests
}

void TestSlackAPI::cleanup()
{
    delete m_api;
    m_api = nullptr;
    delete m_server;
    m_server = nullptr;
}

void TestSlackAPI::authenticate()
{
    QJsonObject response;
    response["ok"] = true;
    response["user_id"] = "U_ME";
    response["team"] = "Testspace";
    response["team_id"] = "T_TEST";
    m_server->setResponse("auth.test", response);

    QSignalSpy authSpy(m_api, &SlackAPI::authenticationChanged);
    m_api->authenticate("xoxp-test-token");
    QVERIFY(authSpy.wait(5000));
}

void TestSlackAPI::authTestSuccess()
{
    authenticate();

    QCOMPARE(m_api->isAuthenticated(), true);
    QCOMPARE(m_api->currentUserId(), QString("U_ME"));
    QCOMPARE(m_api->workspaceName(), QString("Testspace"));
    QCOMPARE(m_api->teamId(), QString("T_TEST"));
    QCOMPARE(m_server->requestCount("auth.test"), 1);
}

void TestSlackAPI::authTestFailure()
{
    QJsonObject response;
    response["ok"] = false;
    response["error"] = "invalid_auth";
    m_server->setResponse("auth.test", response);

    QSignalSpy errorSpy(m_api, &SlackAPI::apiError);
    m_api->authenticate("xoxp-bad-token");

    QVERIFY(errorSpy.wait(5000));
    QCOMPARE(errorSpy.first().at(0).toString(), QString("invalid_auth"));
    QCOMPARE(m_api->isAuthenticated(), false);
}

void TestSlackAPI::conversationsSignalPayload()
{
    authenticate();

    QJsonObject channel1;
    channel1["id"] = "C1";
    channel1["name"] = "general";
    channel1["is_channel"] = true;
    QJsonObject channel2;
    channel2["id"] = "D1";
    channel2["is_im"] = true;
    channel2["user"] = "U42";

    QJsonArray channels;
    channels.append(channel1);
    channels.append(channel2);
    QJsonObject response;
    response["ok"] = true;
    response["channels"] = channels;
    m_server->setResponse("users.conversations", response);

    QSignalSpy conversationsSpy(m_api, &SlackAPI::conversationsReceived);
    m_api->fetchConversations();

    QVERIFY(conversationsSpy.wait(5000));
    QJsonArray payload = conversationsSpy.first().at(0).toJsonArray();
    QCOMPARE(payload.count(), 2);
    QCOMPARE(payload.at(0).toObject()["id"].toString(), QString("C1"));
    QCOMPARE(payload.at(1).toObject()["user"].toString(), QString("U42"));
}

void TestSlackAPI::unreadExtractionFromDisplayCount()
{
    authenticate();

    QJsonObject latest;
    latest["ts"] = "1700000000.000100";
    QJsonObject channel;
    channel["id"] = "C1";
    channel["name"] = "general";
    channel["unread_count_display"] = 3;
    channel["latest"] = latest;
    QJsonObject response;
    response["ok"] = true;
    response["channel"] = channel;
    m_server->setResponse("conversations.info", response);

    QSignalSpy unreadSpy(m_api, &SlackAPI::conversationUnreadReceived);
    QSignalSpy allDoneSpy(m_api, &SlackAPI::allUnreadsFetched);
    m_api->fetchConversationUnreads(QStringList() << "C1");

    QVERIFY(unreadSpy.wait(5000));
    QCOMPARE(unreadSpy.first().at(0).toString(), QString("C1"));
    QCOMPARE(unreadSpy.first().at(1).toInt(), 3);
    QCOMPARE(unreadSpy.first().at(2).toLongLong(), Q_INT64_C(1700000000000));
    QCOMPARE(allDoneSpy.count(), 1);
}

void TestSlackAPI::unreadExtractionNullLatest()
{
    authenticate();

    QJsonObject channel;
    channel["id"] = "C1";
    channel["name"] = "empty-chan";
    channel["unread_count"] = 1;
    channel["latest"] = QJsonValue::Null;
    QJsonObject response;
    response["ok"] = true;
    response["channel"] = channel;
    m_server->setResponse("conversations.info", response);

    QSignalSpy unreadSpy(m_api, &SlackAPI::conversationUnreadReceived);
    m_api->fetchConversationUnreads(QStringList() << "C1");

    QVERIFY(unreadSpy.wait(5000));
    QCOMPARE(unreadSpy.first().at(1).toInt(), 1);
    QCOMPARE(unreadSpy.first().at(2).toLongLong(), Q_INT64_C(0));  // null latest -> no timestamp
}

void TestSlackAPI::batchPacingOfUnreadFetches()
{
    authenticate();

    QJsonObject channel;
    channel["id"] = "C";
    channel["unread_count"] = 0;
    QJsonObject response;
    response["ok"] = true;
    response["channel"] = channel;
    m_server->setResponse("conversations.info", response);

    QSignalSpy unreadSpy(m_api, &SlackAPI::conversationUnreadReceived);
    QSignalSpy allDoneSpy(m_api, &SlackAPI::allUnreadsFetched);

    m_api->fetchConversationUnreads(QStringList() << "C1" << "C2" << "C3" << "C4");

    // First batch: exactly 2 requests go out
    QVERIFY(unreadSpy.wait(5000));
    while (unreadSpy.count() < 2) {
        QVERIFY(unreadSpy.wait(5000));
    }
    QCOMPARE(m_server->requestCount("conversations.info"), 2);
    QCOMPARE(allDoneSpy.count(), 0);

    // Second batch is delayed (~1s), then completion is signalled
    QVERIFY(allDoneSpy.wait(5000));
    QCOMPARE(m_server->requestCount("conversations.info"), 4);
    QCOMPARE(unreadSpy.count(), 4);
}

void TestSlackAPI::okFalseEmitsApiError()
{
    authenticate();

    QJsonObject response;
    response["ok"] = false;
    response["error"] = "rate_limited";
    m_server->setResponse("users.conversations", response);

    QSignalSpy errorSpy(m_api, &SlackAPI::apiError);
    m_api->fetchConversations();

    QVERIFY(errorSpy.wait(5000));
    QCOMPARE(errorSpy.first().at(0).toString(), QString("rate_limited"));
}

void TestSlackAPI::postAndGetRouting()
{
    authenticate();
    m_server->clearRequests();

    // chat.postMessage goes out as POST with a JSON body
    QJsonObject postResponse;
    postResponse["ok"] = true;
    m_server->setResponse("chat.postMessage", postResponse);

    QSignalSpy errorSpy(m_api, &SlackAPI::apiError);
    m_api->sendMessage("C1", "hello world");
    QTRY_COMPARE_WITH_TIMEOUT(m_server->requestCount("chat.postMessage"), 1, 5000);

    MockSlackServer::Request post = m_server->requests().last();
    QCOMPARE(post.method, QString("POST"));
    QJsonObject body = QJsonDocument::fromJson(post.body).object();
    QCOMPARE(body["channel"].toString(), QString("C1"));
    QCOMPARE(body["text"].toString(), QString("hello world"));

    // users.conversations goes out as GET with query parameters
    QJsonObject listResponse;
    listResponse["ok"] = true;
    listResponse["channels"] = QJsonArray();
    m_server->setResponse("users.conversations", listResponse);

    m_api->fetchConversations();
    QTRY_COMPARE_WITH_TIMEOUT(m_server->requestCount("users.conversations"), 1, 5000);

    MockSlackServer::Request get = m_server->requests().last();
    QCOMPARE(get.method, QString("GET"));
    QVERIFY(get.query.hasQueryItem("types"));
    QCOMPARE(get.query.queryItemValue("limit"), QString("200"));
    QVERIFY(get.body.isEmpty());

    QCOMPARE(errorSpy.count(), 0);
}

void TestSlackAPI::unreadResyncFlagLifecycle()
{
    // Login arms the one-shot full unread fetch
    authenticate();
    QCOMPARE(m_api->unreadResyncNeeded(), true);

    QJsonObject channel;
    channel["id"] = "C1";
    channel["unread_count"] = 0;
    QJsonObject response;
    response["ok"] = true;
    response["channel"] = channel;
    m_server->setResponse("conversations.info", response);

    QSignalSpy allDoneSpy(m_api, &SlackAPI::allUnreadsFetched);
    m_api->fetchConversationUnreads(QStringList() << "C1");

    // The fetch consumes the flag: no batch storm on later list refreshes
    QCOMPARE(m_api->unreadResyncNeeded(), false);
    QVERIFY(allDoneSpy.wait(5000));
}

QTEST_GUILESS_MAIN(TestSlackAPI)
#include "tst_slackapi.moc"
