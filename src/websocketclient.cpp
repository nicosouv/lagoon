#include "websocketclient.h"
#include <QJsonDocument>
#include <QDebug>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_isConnected(false)
    , m_reconnectEnabled(false)
    , m_reconnectDelayMs(RECONNECT_MIN_DELAY_MS)
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

    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &WebSocketClient::onReconnectTimeout);
}

WebSocketClient::~WebSocketClient()
{
    close();
}

void WebSocketClient::connectToUrl(const QString &url)
{
    // Connecting (or reconnecting) re-arms auto-reconnect
    m_reconnectEnabled = true;
    m_reconnectTimer->stop();

    if (m_webSocket->state() != QAbstractSocket::UnconnectedState) {
        m_webSocket->abort();
    }

    qDebug() << "Connecting to WebSocket";
    m_webSocket->open(QUrl(url));
}

void WebSocketClient::close()
{
    m_reconnectEnabled = false;
    m_reconnectTimer->stop();
    stopPingTimer();

    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
}

void WebSocketClient::scheduleReconnect()
{
    if (!m_reconnectEnabled || m_reconnectTimer->isActive()) {
        return;
    }

    qDebug() << "WebSocket reconnect scheduled in" << m_reconnectDelayMs << "ms";
    m_reconnectTimer->start(m_reconnectDelayMs);
    m_reconnectDelayMs = qMin(m_reconnectDelayMs * 2, RECONNECT_MAX_DELAY_MS);
}

void WebSocketClient::onReconnectTimeout()
{
    if (!m_reconnectEnabled || m_isConnected) {
        return;
    }
    qDebug() << "WebSocket reconnecting...";
    emit reconnectNeeded();
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
    m_reconnectDelayMs = RECONNECT_MIN_DELAY_MS;  // reset backoff
    m_reconnectTimer->stop();
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
    scheduleReconnect();
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
    Q_UNUSED(socketError);
    QString errorString = m_webSocket->errorString();
    qWarning() << "WebSocket error:" << errorString;
    emit error(errorString);

    scheduleReconnect();
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
