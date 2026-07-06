#include "notificationcoordinator.h"
#include "models/conversationmodel.h"
#include "models/usermodel.h"

NotificationCoordinator::NotificationCoordinator(QObject *parent)
    : QObject(parent)
    , m_conversations(nullptr)
    , m_users(nullptr)
{
}

void NotificationCoordinator::setModels(ConversationModel *conversations, UserModel *users)
{
    m_conversations = conversations;
    m_users = users;
}

void NotificationCoordinator::setCurrentUserId(const QString &userId)
{
    m_currentUserId = userId;
}

void NotificationCoordinator::setActiveChannelId(const QString &channelId)
{
    m_activeChannelId = channelId;
}

bool NotificationCoordinator::isMention(const QString &text, const QString &userId)
{
    if (userId.isEmpty()) {
        return false;
    }
    return text.contains(QStringLiteral("<@%1>").arg(userId));
}

QString NotificationCoordinator::channelDisplayName(const QString &channelId) const
{
    if (m_conversations) {
        for (int i = 0; i < m_conversations->rowCount(); ++i) {
            QModelIndex index = m_conversations->index(i, 0);
            if (m_conversations->data(index, ConversationModel::IdRole).toString() == channelId) {
                // NameRole resolves DM names through the user model
                return m_conversations->data(index, ConversationModel::NameRole).toString();
            }
        }
    }
    return channelId;
}

void NotificationCoordinator::handleRtmMessage(const QJsonObject &message)
{
    QString channelId = message["channel"].toString();
    QString userId = message["user"].toString();
    QString text = message["text"].toString();

    if (channelId.isEmpty() || userId.isEmpty()) {
        return;
    }

    // Own messages echo back through RTM: never notify about ourselves
    if (userId == m_currentUserId) {
        return;
    }

    // The user is already reading this conversation
    if (!m_activeChannelId.isEmpty() && channelId == m_activeChannelId) {
        return;
    }

    QString channelName = channelDisplayName(channelId);
    QString userName = m_users ? m_users->getUserName(userId) : userId;

    if (isMention(text, m_currentUserId)) {
        emit mentionNotification(channelName, userName, text, channelId);
    } else {
        emit messageNotification(channelName, userName, text, channelId);
    }
}
