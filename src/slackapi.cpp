#include "slackapi.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QDebug>

const QString SlackAPI::API_BASE_URL = "https://slack.com/api/";

SlackAPI::SlackAPI(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_webSocketClient(new WebSocketClient(this))
    , m_isAuthenticated(false)
    , m_refreshTimer(new QTimer(this))
    , m_autoRefresh(true)
    , m_refreshInterval(30)  // 30 seconds by default
    , m_sessionBandwidthBytes(0)  // Initialize session bandwidth
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &SlackAPI::handleNetworkReply);

    connect(m_webSocketClient, &WebSocketClient::messageReceived,
            this, &SlackAPI::handleWebSocketMessage);
    connect(m_webSocketClient, &WebSocketClient::error,
            this, &SlackAPI::handleWebSocketError);

    // Setup refresh timer
    m_refreshTimer->setInterval(m_refreshInterval * 1000);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &SlackAPI::handleRefreshTimer);
}

SlackAPI::~SlackAPI()
{
}

void SlackAPI::authenticate(const QString &token)
{
    m_token = token;
    emit tokenChanged();

    qDebug() << "=== AUTHENTICATE CALLED ===";
    qDebug() << "Token length:" << m_token.length();

    // Test authentication with auth.test endpoint
    QUrl url(API_BASE_URL + "auth.test");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "Sending auth.test request to:" << url.toString();
    m_networkManager->get(request);
}

void SlackAPI::logout()
{
    m_token.clear();
    m_workspaceName.clear();
    m_teamId.clear();
    m_currentUserId.clear();
    m_isAuthenticated = false;

    // Stop auto-refresh timer
    m_refreshTimer->stop();
    m_lastUnreadCounts.clear();

    disconnectWebSocket();

    emit tokenChanged();
    emit workspaceChanged();
    emit teamIdChanged();
    emit authenticationChanged();
}

void SlackAPI::fetchConversations()
{
    QJsonObject params;
    params["types"] = "public_channel,private_channel,mpim,im";
    params["limit"] = 200;
    params["exclude_archived"] = true;

    // Use users.conversations instead of conversations.list
    // This returns user-specific data like last_read timestamps
    makeApiRequest("users.conversations", params);
}

void SlackAPI::fetchConversationHistory(const QString &channelId, int limit)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["limit"] = limit;

    makeApiRequest("conversations.history", params);
}

void SlackAPI::joinConversation(const QString &channelId)
{
    QJsonObject params;
    params["channel"] = channelId;

    makeApiRequest("conversations.join", params);
}

void SlackAPI::leaveConversation(const QString &channelId)
{
    QJsonObject params;
    params["channel"] = channelId;

    makeApiRequest("conversations.leave", params);
}

void SlackAPI::sendMessage(const QString &channelId, const QString &text)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["text"] = text;

    makeApiRequest("chat.postMessage", params);
}

void SlackAPI::sendThreadReply(const QString &channelId, const QString &threadTs, const QString &text)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["thread_ts"] = threadTs;
    params["text"] = text;

    makeApiRequest("chat.postMessage", params);
}

void SlackAPI::fetchThreadReplies(const QString &channelId, const QString &threadTs)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["ts"] = threadTs;

    makeApiRequest("conversations.replies", params);
}

void SlackAPI::updateMessage(const QString &channelId, const QString &ts, const QString &text)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["ts"] = ts;
    params["text"] = text;

    makeApiRequest("chat.update", params);
}

void SlackAPI::deleteMessage(const QString &channelId, const QString &ts)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["ts"] = ts;

    makeApiRequest("chat.delete", params);
}

void SlackAPI::addReaction(const QString &channelId, const QString &ts, const QString &emoji)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["timestamp"] = ts;
    params["name"] = emoji;

    makeApiRequest("reactions.add", params);
}

void SlackAPI::removeReaction(const QString &channelId, const QString &ts, const QString &emoji)
{
    QJsonObject params;
    params["channel"] = channelId;
    params["timestamp"] = ts;
    params["name"] = emoji;

    makeApiRequest("reactions.remove", params);
}

void SlackAPI::fetchUsers()
{
    makeApiRequest("users.list");
}

void SlackAPI::fetchUserInfo(const QString &userId)
{
    QJsonObject params;
    params["user"] = userId;

    makeApiRequest("users.info", params);
}

void SlackAPI::connectWebSocket()
{
    if (m_token.isEmpty()) {
        qWarning() << "Cannot connect WebSocket: not authenticated";
        return;
    }

    qDebug() << "Requesting RTM WebSocket URL...";
    // Use rtm.connect for user tokens (xoxp-) instead of apps.connections.open (which requires bot tokens)
    makeApiRequest("rtm.connect");
}

void SlackAPI::disconnectWebSocket()
{
    m_webSocketClient->disconnect();
}

void SlackAPI::handleNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();

    qDebug() << "=== NETWORK REPLY RECEIVED ===";
    qDebug() << "URL:" << reply->url().toString();
    qDebug() << "Error:" << reply->error();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network error:" << reply->errorString();
        emit networkError(reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();

    // Track bandwidth (received bytes)
    qint64 bytesReceived = data.size();
    // Also count request header size (approximate)
    qint64 requestSize = reply->request().url().toString().toUtf8().size() + 200; // ~200 bytes for headers
    trackBandwidth(bytesReceived + requestSize);

    qDebug() << "Response data:" << data;
    qDebug() << "Bytes received:" << bytesReceived << "+ request:" << requestSize << "=" << (bytesReceived + requestSize);

    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        qDebug() << "Response is not a JSON object";
        emit apiError("Invalid response from Slack API");
        return;
    }

    QJsonObject response = doc.object();
    qDebug() << "Response 'ok' field:" << response["ok"].toBool();

    if (!response["ok"].toBool()) {
        QString error = response["error"].toString();
        qDebug() << "API error:" << error;
        emit apiError(error);
        return;
    }

    // Extract endpoint from reply URL
    QString endpoint = reply->url().path();
    qDebug() << "Full path:" << endpoint;
    endpoint.remove("/api/");
    qDebug() << "Extracted endpoint:" << endpoint;

    processApiResponse(endpoint, response);
}

void SlackAPI::handleWebSocketMessage(const QJsonObject &message)
{
    QString type = message["type"].toString();

    if (type == "message") {
        emit messageReceived(message);
    } else if (type == "message_changed") {
        emit messageUpdated(message);
    } else if (type == "message_deleted") {
        QString channelId = message["channel"].toString();
        QString ts = message["deleted_ts"].toString();
        emit messageDeleted(channelId, ts);
    } else if (type == "reaction_added") {
        emit reactionAdded(message);
    } else if (type == "reaction_removed") {
        emit reactionRemoved(message);
    }
}

void SlackAPI::handleWebSocketError(const QString &error)
{
    qWarning() << "WebSocket error:" << error;
    emit networkError(error);
}

void SlackAPI::makeApiRequest(const QString &endpoint, const QJsonObject &params)
{
    if (m_token.isEmpty() && endpoint != "auth.test") {
        emit apiError("Not authenticated");
        return;
    }

    QUrl url(API_BASE_URL + endpoint);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Endpoints that require POST with JSON body
    bool requiresPost = endpoint.startsWith("chat.") ||
                       endpoint.startsWith("files.") ||
                       endpoint.startsWith("reactions.");

    if (requiresPost) {
        // POST request with JSON body
        QJsonDocument doc(params);
        QByteArray jsonData = doc.toJson();
        qDebug() << "Sending POST request to" << endpoint << "with body:" << jsonData;
        m_networkManager->post(request, jsonData);
    } else {
        // GET request with query parameters
        if (!params.isEmpty()) {
            QUrlQuery query;
            for (auto it = params.begin(); it != params.end(); ++it) {
                query.addQueryItem(it.key(), it.value().toVariant().toString());
            }
            url.setQuery(query);
            request.setUrl(url);
        }
        m_networkManager->get(request);
    }
}

void SlackAPI::processApiResponse(const QString &endpoint, const QJsonObject &response)
{
    qDebug() << "=== PROCESS API RESPONSE ===";
    qDebug() << "Endpoint:" << endpoint;
    qDebug() << "Checking if endpoint == 'auth.test':" << (endpoint == "auth.test");

    if (endpoint == "auth.test") {
        qDebug() << "AUTH.TEST response received!";
        qDebug() << "user_id:" << response["user_id"].toString();
        qDebug() << "team:" << response["team"].toString();
        qDebug() << "team_id:" << response["team_id"].toString();

        m_isAuthenticated = true;
        m_currentUserId = response["user_id"].toString();
        m_workspaceName = response["team"].toString();
        m_teamId = response["team_id"].toString();

        qDebug() << "Setting m_isAuthenticated to true";
        qDebug() << "Emitting authenticationChanged signal";

        emit authenticationChanged();
        emit workspaceChanged();
        emit teamIdChanged();
        emit currentUserChanged();

        // After authentication, start auto-refresh timer if enabled
        if (m_autoRefresh) {
            m_refreshTimer->start();
            qDebug() << "Auto-refresh started, polling every" << m_refreshInterval << "seconds";
        }

        // After authentication, connect WebSocket
        connectWebSocket();

    } else if (endpoint == "users.conversations" || endpoint == "conversations.list") {
        QJsonArray conversations = response["channels"].toArray();

        // Detect new unread messages for notifications
        for (const QJsonValue &value : conversations) {
            if (value.isObject()) {
                QJsonObject conv = value.toObject();
                QString channelId = conv["id"].toString();

                // For DMs/MPIMs: use unread_count_display if available
                // For channels: calculate by comparing last_read with latest message
                int unreadCount = 0;

                if (conv.contains("unread_count_display")) {
                    // DMs and group messages provide this
                    unreadCount = conv["unread_count_display"].toInt();
                } else if (conv.contains("last_read")) {
                    // For channels, check if there are unread messages
                    QString lastRead = conv["last_read"].toString();
                    QJsonObject latest = conv["latest"].toObject();
                    QString latestTs = latest["ts"].toString();

                    // If latest message timestamp > last_read, there are unread messages
                    if (!latestTs.isEmpty() && !lastRead.isEmpty()) {
                        double lastReadDouble = lastRead.toDouble();
                        double latestDouble = latestTs.toDouble();
                        if (latestDouble > lastReadDouble) {
                            unreadCount = 1;  // We don't know exact count, but we know there are unreads
                        }
                    }
                } else {
                    // Fallback to unread_count if present (though it's usually not for channels)
                    unreadCount = conv["unread_count"].toInt();
                }

                // Check if this is a new unread message
                if (unreadCount > 0 && m_lastUnreadCounts.contains(channelId)) {
                    int lastCount = m_lastUnreadCounts.value(channelId);
                    if (unreadCount > lastCount) {
                        // New unread messages detected!
                        qDebug() << "New unread messages in channel" << channelId << ":" << (unreadCount - lastCount);
                        emit newUnreadMessages(channelId, unreadCount - lastCount);
                    }
                }

                // Update stored unread count
                m_lastUnreadCounts.insert(channelId, unreadCount);
            }
        }

        emit conversationsReceived(conversations);

    } else if (endpoint == "conversations.history") {
        QJsonArray messages = response["messages"].toArray();
        qDebug() << "CONVERSATIONS.HISTORY: Emitting messagesReceived signal with" << messages.count() << "messages";
        emit messagesReceived(messages);

    } else if (endpoint == "conversations.replies") {
        QJsonArray messages = response["messages"].toArray();
        qDebug() << "CONVERSATIONS.REPLIES: Emitting threadRepliesReceived signal with" << messages.count() << "messages";
        emit threadRepliesReceived(messages);

    } else if (endpoint == "users.list") {
        QJsonArray users = response["members"].toArray();
        emit usersReceived(users);

    } else if (endpoint == "users.info") {
        QJsonObject user = response["user"].toObject();
        emit userInfoReceived(user);

    } else if (endpoint == "rtm.connect") {
        qDebug() << "RTM connect response received";
        QString wsUrl = response["url"].toString();
        if (!wsUrl.isEmpty()) {
            qDebug() << "Connecting to RTM WebSocket URL:" << wsUrl;
            m_webSocketClient->connectToUrl(wsUrl);
        } else {
            qDebug() << "No WebSocket URL in rtm.connect response";
        }
    }
}

void SlackAPI::setAutoRefresh(bool enabled)
{
    if (m_autoRefresh != enabled) {
        m_autoRefresh = enabled;
        emit autoRefreshChanged();

        if (m_autoRefresh && m_isAuthenticated) {
            m_refreshTimer->start();
            qDebug() << "Auto-refresh enabled, polling every" << m_refreshInterval << "seconds";
        } else {
            m_refreshTimer->stop();
            qDebug() << "Auto-refresh disabled";
        }
    }
}

void SlackAPI::setRefreshInterval(int seconds)
{
    if (m_refreshInterval != seconds && seconds > 0) {
        m_refreshInterval = seconds;
        m_refreshTimer->setInterval(m_refreshInterval * 1000);
        emit refreshIntervalChanged();

        qDebug() << "Refresh interval changed to" << m_refreshInterval << "seconds";

        // Restart timer if it's running
        if (m_refreshTimer->isActive()) {
            m_refreshTimer->start();
        }
    }
}

void SlackAPI::handleRefreshTimer()
{
    if (!m_isAuthenticated) {
        return;
    }

    qDebug() << "Auto-refresh: fetching conversations...";
    fetchConversations();
}

void SlackAPI::trackBandwidth(qint64 bytes)
{
    // Update session bandwidth
    m_sessionBandwidthBytes += bytes;
    emit sessionBandwidthBytesChanged();

    // Signal for total bandwidth update (connected to AppSettings in main.cpp)
    emit bandwidthBytesAdded(bytes);
}
