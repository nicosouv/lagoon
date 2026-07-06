#ifndef NOTIFICATIONCOORDINATOR_H
#define NOTIFICATIONCOORDINATOR_H

#include <QJsonObject>
#include <QObject>
#include <QString>

class ConversationModel;
class UserModel;

// Decides which RTM messages become notifications: skips own messages and
// the conversation currently on screen, and detects real mentions. Emits
// signals instead of calling NotificationManager so it stays unit-testable
// (nemonotifications is not available off-device); main.cpp does the wiring.
class NotificationCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit NotificationCoordinator(QObject *parent = nullptr);

    void setModels(ConversationModel *conversations, UserModel *users);
    void setCurrentUserId(const QString &userId);
    void setActiveChannelId(const QString &channelId);

    // A real Slack mention is <@USERID>, not any '@' in the text
    static bool isMention(const QString &text, const QString &userId);

public slots:
    void handleRtmMessage(const QJsonObject &message);

signals:
    void messageNotification(const QString &channelName, const QString &userName,
                             const QString &text, const QString &channelId);
    void mentionNotification(const QString &channelName, const QString &userName,
                             const QString &text, const QString &channelId);

private:
    QString channelDisplayName(const QString &channelId) const;

    ConversationModel *m_conversations;
    UserModel *m_users;
    QString m_currentUserId;
    QString m_activeChannelId;
};

#endif // NOTIFICATIONCOORDINATOR_H
