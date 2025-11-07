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

public slots:
    void connectToUrl(const QString &url);
    void disconnect();
    void sendMessage(const QJsonObject &message);

signals:
    void connectionChanged();
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void error(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onPingTimeout();

private:
    void startPingTimer();
    void stopPingTimer();

    QWebSocket *m_webSocket;
    QTimer *m_pingTimer;
    bool m_isConnected;
    int m_messageId;
};

#endif // WEBSOCKETCLIENT_H
