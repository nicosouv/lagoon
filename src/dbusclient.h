#ifndef DBUSCLIENT_H
#define DBUSCLIENT_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>

class DBusClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isDaemonRunning READ isDaemonRunning NOTIFY daemonStatusChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)

public:
    explicit DBusClient(QObject *parent = nullptr);
    ~DBusClient();

    bool isDaemonRunning() const { return m_isDaemonRunning; }
    bool isConnected() const { return m_isConnected; }
    int unreadCount() const { return m_unreadCount; }

public slots:
    // Call daemon methods
    void syncNow();
    void setWorkspace(const QString &workspaceId);
    void markChannelAsRead(const QString &channelId);
    void sendMessage(const QString &channelId, const QString &text);

    // Start/stop daemon
    void startDaemon();
    void checkDaemonStatus();

signals:
    // Signals from daemon
    void newMessageReceived(const QString &channelId, const QString &messageJson);
    void unreadCountChanged(int totalUnread);
    void connectionStateChanged(bool connected);
    void syncCompleted();
    void daemonStatusChanged();

private slots:
    void handleNewMessageReceived(const QString &channelId, const QString &messageJson);
    void handleUnreadCountChanged(int totalUnread);
    void handleConnectionStateChanged(bool connected);
    void handleSyncCompleted();

private:
    void connectToSignals();

    QDBusInterface *m_interface;
    bool m_isDaemonRunning;
    bool m_isConnected;
    int m_unreadCount;
};

#endif // DBUSCLIENT_H
