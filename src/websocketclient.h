#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QTimer>

class WebSocketClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)

public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    bool isConnected() const { return m_isConnected; }

    // Schedule a reconnect attempt with exponential backoff. Also called by
    // SlackAPI when a rtm.connect request fails so retrying never stalls.
    void scheduleReconnect();

public slots:
    void connectToUrl(const QString &url);
    void close();  // Intentional disconnect: disables auto-reconnect
    void sendMessage(const QJsonObject &message);

signals:
    void connectionChanged();
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void error(const QString &error);
    // The wss URL from rtm.connect expires: SlackAPI must call rtm.connect
    // again and connectToUrl() with the fresh URL
    void reconnectNeeded();

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onPingTimeout();
    void onReconnectTimeout();

private:
    void startPingTimer();
    void stopPingTimer();

    QWebSocket *m_webSocket;
    QTimer *m_pingTimer;
    QTimer *m_reconnectTimer;
    bool m_isConnected;
    bool m_reconnectEnabled;
    int m_reconnectDelayMs;  // exponential backoff: 1s, 2s, 4s... capped at 60s
    int m_messageId;

    static const int RECONNECT_MIN_DELAY_MS = 1000;
    static const int RECONNECT_MAX_DELAY_MS = 60000;
};

#endif // WEBSOCKETCLIENT_H
