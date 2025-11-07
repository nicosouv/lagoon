#include "websocketclient.h"
#include <QJsonDocument>
#include <QDebug>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_pingTimer(new QTimer(this))
    , m_isConnected(false)
    , m_messageId(1)
{
    connect(m_webSocket, &QWebSocket::connected,
            this, &WebSocketClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected,
            this, &WebSocketClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessageReceived);
    connect(m_webSocket, static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),
            this, &WebSocketClient::onError);

    // Setup ping timer for keeping connection alive
    m_pingTimer->setInterval(30000); // 30 seconds
    connect(m_pingTimer, &QTimer::timeout,
            this, &WebSocketClient::onPingTimeout);
}

WebSocketClient::~WebSocketClient()
{
    disconnect();
}

void WebSocketClient::connectToUrl(const QString &url)
{
    if (m_isConnected) {
        disconnect();
    }

    qDebug() << "Connecting to WebSocket:" << url;
    m_webSocket->open(QUrl(url));
}

void WebSocketClient::disconnect()
{
    stopPingTimer();

    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
}

void WebSocketClient::sendMessage(const QJsonObject &message)
{
    if (!m_isConnected) {
        qWarning() << "Cannot send message: not connected";
        return;
    }

    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    m_webSocket->sendTextMessage(jsonString);
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket connected";
    m_isConnected = true;
    emit connectionChanged();
    emit connected();

    startPingTimer();
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "WebSocket disconnected";
    m_isConnected = false;
    emit connectionChanged();
    emit disconnected();

    stopPingTimer();
}

void WebSocketClient::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

    if (!doc.isObject()) {
        qWarning() << "Received invalid JSON from WebSocket";
        return;
    }

    QJsonObject jsonObject = doc.object();

    // Handle different message types
    QString type = jsonObject["type"].toString();

    if (type == "hello") {
        qDebug() << "Received hello from Slack RTM";
    } else if (type == "error") {
        QString errorMsg = jsonObject["error"].toObject()["msg"].toString();
        emit error(errorMsg);
    } else {
        emit messageReceived(jsonObject);
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError socketError)
{
    QString errorString = m_webSocket->errorString();
    qWarning() << "WebSocket error:" << errorString;
    emit error(errorString);
}

void WebSocketClient::onPingTimeout()
{
    // Send ping message to keep connection alive
    QJsonObject pingMessage;
    pingMessage["type"] = "ping";
    pingMessage["id"] = m_messageId++;

    sendMessage(pingMessage);
}

void WebSocketClient::startPingTimer()
{
    m_pingTimer->start();
}

void WebSocketClient::stopPingTimer()
{
    m_pingTimer->stop();
}
