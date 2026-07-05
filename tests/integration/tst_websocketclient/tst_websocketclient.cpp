#include <QtTest>
#include <QJsonObject>
#include <QSignalSpy>
#include <QWebSocket>
#include <QWebSocketServer>

#include "websocketclient.h"

class TestWebSocketClient : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void messageDispatch();
    void helloIsNotForwarded();
    void errorTypeEmitsError();
    void reconnectAfterServerDrop();
    void closePreventsReconnect();
    void reconnectWhenServerUnreachable();

private:
    QWebSocket *waitForServerSocket();
    QString serverUrl() const;

    QWebSocketServer *m_server;
    WebSocketClient *m_client;
};

void TestWebSocketClient::init()
{
    m_server = new QWebSocketServer("mock-rtm", QWebSocketServer::NonSecureMode);
    QVERIFY(m_server->listen(QHostAddress::LocalHost, 0));

    m_client = new WebSocketClient;
}

void TestWebSocketClient::cleanup()
{
    delete m_client;
    m_client = nullptr;
    delete m_server;
    m_server = nullptr;
}

QString TestWebSocketClient::serverUrl() const
{
    return QString("ws://127.0.0.1:%1").arg(m_server->serverPort());
}

QWebSocket *TestWebSocketClient::waitForServerSocket()
{
    QSignalSpy connectedSpy(m_client, &WebSocketClient::connected);
    m_client->connectToUrl(serverUrl());

    if (!m_server->hasPendingConnections()) {
        QSignalSpy newConnectionSpy(m_server, &QWebSocketServer::newConnection);
        if (!newConnectionSpy.wait(5000)) {
            return nullptr;
        }
    }
    if (connectedSpy.isEmpty() && !connectedSpy.wait(5000)) {
        return nullptr;
    }

    QWebSocket *socket = m_server->nextPendingConnection();
    socket->setParent(m_server);
    return socket;
}

void TestWebSocketClient::messageDispatch()
{
    QWebSocket *serverSocket = waitForServerSocket();
    QVERIFY(serverSocket);
    QVERIFY(m_client->isConnected());

    QSignalSpy messageSpy(m_client, &WebSocketClient::messageReceived);
    serverSocket->sendTextMessage("{\"type\":\"message\",\"channel\":\"C1\",\"text\":\"hi\"}");

    QVERIFY(messageSpy.wait(5000));
    QJsonObject payload = messageSpy.first().at(0).toJsonObject();
    QCOMPARE(payload["type"].toString(), QString("message"));
    QCOMPARE(payload["channel"].toString(), QString("C1"));
}

void TestWebSocketClient::helloIsNotForwarded()
{
    QWebSocket *serverSocket = waitForServerSocket();
    QVERIFY(serverSocket);

    QSignalSpy messageSpy(m_client, &WebSocketClient::messageReceived);
    serverSocket->sendTextMessage("{\"type\":\"hello\"}");

    QTest::qWait(300);
    QCOMPARE(messageSpy.count(), 0);
}

void TestWebSocketClient::errorTypeEmitsError()
{
    QWebSocket *serverSocket = waitForServerSocket();
    QVERIFY(serverSocket);

    QSignalSpy errorSpy(m_client, &WebSocketClient::error);
    serverSocket->sendTextMessage("{\"type\":\"error\",\"error\":{\"msg\":\"account_inactive\"}}");

    QVERIFY(errorSpy.wait(5000));
    QCOMPARE(errorSpy.first().at(0).toString(), QString("account_inactive"));
}

void TestWebSocketClient::reconnectAfterServerDrop()
{
    QWebSocket *serverSocket = waitForServerSocket();
    QVERIFY(serverSocket);

    QSignalSpy disconnectedSpy(m_client, &WebSocketClient::disconnected);
    QSignalSpy reconnectSpy(m_client, &WebSocketClient::reconnectNeeded);

    serverSocket->close();

    QVERIFY(disconnectedSpy.wait(5000));
    // First backoff step is 1s
    QVERIFY(reconnectSpy.wait(5000));
    QCOMPARE(reconnectSpy.count(), 1);
}

void TestWebSocketClient::closePreventsReconnect()
{
    QWebSocket *serverSocket = waitForServerSocket();
    QVERIFY(serverSocket);

    QSignalSpy disconnectedSpy(m_client, &WebSocketClient::disconnected);
    QSignalSpy reconnectSpy(m_client, &WebSocketClient::reconnectNeeded);

    m_client->close();

    // close() may emit disconnected synchronously: only wait if it has not
    if (disconnectedSpy.isEmpty()) {
        QVERIFY(disconnectedSpy.wait(5000));
    }
    QTest::qWait(1500);  // longer than the first backoff step
    QCOMPARE(reconnectSpy.count(), 0);
}

void TestWebSocketClient::reconnectWhenServerUnreachable()
{
    // Point the client at a closed port: connection fails, backoff kicks in
    quint16 port = m_server->serverPort();
    m_server->close();

    QSignalSpy reconnectSpy(m_client, &WebSocketClient::reconnectNeeded);
    m_client->connectToUrl(QString("ws://127.0.0.1:%1").arg(port));

    QVERIFY(reconnectSpy.wait(5000));
}

QTEST_GUILESS_MAIN(TestWebSocketClient)
#include "tst_websocketclient.moc"
