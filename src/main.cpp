#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QScopedPointer>
#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>
#include <QTranslator>
#include <QLocale>

#include "slackapi.h"
#include "slackimageprovider.h"
#include "notificationmanager.h"
#include "notificationcoordinator.h"
#include "workspacemanager.h"
#include "filemanager.h"
#include "oauthmanager.h"
#include "statsmanager.h"
#include "updatechecker.h"
#include "dbusinterface.h"
#include "draftmanager.h"
#include "models/conversationmodel.h"
#include "models/messagemodel.h"
#include "models/usermodel.h"
#include "settings/appsettings.h"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    app->setOrganizationName("harbour-lagoon");
    app->setApplicationName("harbour-lagoon");

    // Load translation
    QScopedPointer<QTranslator> translator(new QTranslator(app.data()));
    AppSettings tempSettings; // Temporary settings object to read language preference
    QString language = tempSettings.language();

    // If no language is set, use system locale
    if (language.isEmpty()) {
        language = QLocale::system().name();
    }

    // Try to load the translation file
    QString translationFile = QString("harbour-lagoon-%1").arg(language);
    if (translator->load(translationFile, SailfishApp::pathTo("translations").toLocalFile())) {
        app->installTranslator(translator.data());
    } else {
        // Try loading just the language code (e.g., "fr" instead of "fr_FR")
        QString shortLang = language.left(2);
        translationFile = QString("harbour-lagoon-%1").arg(shortLang);
        if (translator->load(translationFile, SailfishApp::pathTo("translations").toLocalFile())) {
            app->installTranslator(translator.data());
        }
    }

    QScopedPointer<QQuickView> view(SailfishApp::createView());

    // Create and register image provider for authenticated Slack images
    SlackImageProvider *imageProvider = new SlackImageProvider();
    view->engine()->addImageProvider("slack", imageProvider);

    // Create managers
    WorkspaceManager *workspaceManager = new WorkspaceManager(app.data());
    NotificationManager *notificationManager = new NotificationManager(app.data());
    FileManager *fileManager = new FileManager(app.data());
    OAuthManager *oauthManager = new OAuthManager(app.data());
    StatsManager *statsManager = new StatsManager(app.data());
    UpdateChecker *updateChecker = new UpdateChecker(app.data());
    DraftManager *draftManager = new DraftManager(app.data());

    // Create DBus interface for notification clicks
    DBusInterface *dbusInterface = new DBusInterface(app.data());
    DBusAdaptor *dbusAdaptor = dbusInterface->getDBusAdaptor();

    // Create API instance
    SlackAPI *slackAPI = new SlackAPI(app.data());

    // Connect SlackAPI token to image provider
    QObject::connect(slackAPI, &SlackAPI::tokenChanged,
                     [imageProvider, slackAPI]() {
        imageProvider->setToken(slackAPI->token());
    });
    // Set initial token if already authenticated
    if (!slackAPI->token().isEmpty()) {
        imageProvider->setToken(slackAPI->token());
    }

    // Create models
    ConversationModel *conversationModel = new ConversationModel(app.data());
    MessageModel *messageModel = new MessageModel(app.data());
    UserModel *userModel = new UserModel(app.data());

    // Let the conversation model resolve DM names, and refresh them when users load
    conversationModel->setUserModel(userModel);
    QObject::connect(userModel, &UserModel::usersUpdated,
                     conversationModel, &ConversationModel::refreshDmNames);

    // Message texts are formatted in C++; mentions re-resolve when users load
    messageModel->setUserModel(userModel);
    QObject::connect(userModel, &UserModel::usersUpdated,
                     messageModel, &MessageModel::refreshFormatting);

    // Sort the conversation list once after the batch unread fetch completes
    QObject::connect(slackAPI, &SlackAPI::allUnreadsFetched,
                     conversationModel, &ConversationModel::resortAndNotify);

    // Batch unread fetch only after login or an explicit resync; steady-state
    // unreads are maintained locally from RTM events
    QObject::connect(conversationModel, &ConversationModel::conversationsUpdated,
                     slackAPI, [slackAPI](const QStringList &conversationIds) {
        if (!conversationIds.isEmpty() && slackAPI->unreadResyncNeeded()) {
            slackAPI->fetchConversationUnreads(conversationIds);
        }
    });

    // Conversation read on another device
    QObject::connect(slackAPI, &SlackAPI::conversationMarked,
                     conversationModel, &ConversationModel::markAsRead);

    // RTM reactions from other users/devices update the open conversation
    QObject::connect(slackAPI, &SlackAPI::reactionAdded,
                     messageModel, [messageModel](const QJsonObject &event) {
        QJsonObject item = event["item"].toObject();
        if (item["channel"].toString() == messageModel->currentChannelId()) {
            messageModel->applyReaction(item["ts"].toString(),
                                        event["reaction"].toString(),
                                        event["user"].toString(), true);
        }
    });
    QObject::connect(slackAPI, &SlackAPI::reactionRemoved,
                     messageModel, [messageModel](const QJsonObject &event) {
        QJsonObject item = event["item"].toObject();
        if (item["channel"].toString() == messageModel->currentChannelId()) {
            messageModel->applyReaction(item["ts"].toString(),
                                        event["reaction"].toString(),
                                        event["user"].toString(), false);
        }
    });

    // After suspend/resume the RTM socket is dead: reconnect and resync
    QObject::connect(app.data(), &QGuiApplication::applicationStateChanged,
                     slackAPI, [slackAPI](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            slackAPI->handleAppActivated();
        }
    });

    // Create settings
    AppSettings *settings = new AppSettings(app.data());

    // Connect stats manager to API for workspace and user tracking
    // Note: workspace switching and authentication is handled in QML (harbour-lagoon.qml)
    QObject::connect(slackAPI, &SlackAPI::teamIdChanged,
                     statsManager, [statsManager, slackAPI]() {
        statsManager->setCurrentWorkspace(slackAPI->teamId());
    });

    QObject::connect(slackAPI, &SlackAPI::currentUserChanged,
                     statsManager, [statsManager, slackAPI]() {
        statsManager->setCurrentUserId(slackAPI->currentUserId());
    });

    // Connect conversation model to API for starred channels persistence
    QObject::connect(slackAPI, &SlackAPI::teamIdChanged,
                     conversationModel, [conversationModel, slackAPI]() {
        conversationModel->setTeamId(slackAPI->teamId());
    });

    // Connect API to models
    QObject::connect(slackAPI, &SlackAPI::conversationsReceived,
                     conversationModel, &ConversationModel::updateConversations);
    QObject::connect(slackAPI, &SlackAPI::messagesReceived,
                     messageModel, &MessageModel::updateMessages);

    // Also track historical messages for stats
    QObject::connect(slackAPI, &SlackAPI::messagesReceived,
                     statsManager, [statsManager](const QJsonArray &messages) {
        for (const QJsonValue &value : messages) {
            if (value.isObject()) {
                statsManager->trackMessage(value.toObject());
            }
        }
    });

    QObject::connect(slackAPI, &SlackAPI::usersReceived,
                     userModel, [userModel, slackAPI](const QJsonArray &users) {
        // Pass teamId to enable full user cache per workspace
        userModel->updateUsers(users, slackAPI->teamId());
    });

    // Notifications: the coordinator filters own messages, the conversation
    // on screen, and detects real <@USERID> mentions
    NotificationCoordinator *notificationCoordinator = new NotificationCoordinator(app.data());
    notificationCoordinator->setModels(conversationModel, userModel);
    QObject::connect(slackAPI, &SlackAPI::currentUserChanged,
                     notificationCoordinator, [notificationCoordinator, slackAPI]() {
        notificationCoordinator->setCurrentUserId(slackAPI->currentUserId());
    });
    QObject::connect(slackAPI, &SlackAPI::activeChannelIdChanged,
                     notificationCoordinator, [notificationCoordinator, slackAPI]() {
        notificationCoordinator->setActiveChannelId(slackAPI->activeChannelId());
    });
    QObject::connect(slackAPI, &SlackAPI::messageReceived,
                     notificationCoordinator, &NotificationCoordinator::handleRtmMessage);
    QObject::connect(notificationCoordinator, &NotificationCoordinator::messageNotification,
                     notificationManager, &NotificationManager::showMessageNotification);
    QObject::connect(notificationCoordinator, &NotificationCoordinator::mentionNotification,
                     notificationManager, &NotificationManager::showMentionNotification);

    // Track incoming RTM messages in stats
    QObject::connect(slackAPI, &SlackAPI::messageReceived,
                     statsManager, [statsManager](const QJsonObject &message) {
        statsManager->trackMessage(message);
    });

    // Connect notification manager to file manager
    QObject::connect(notificationManager, &NotificationManager::enabledChanged,
                     settings, [settings, notificationManager]() {
        settings->setNotificationsEnabled(notificationManager->enabled());
    });

    // Sync notification settings
    notificationManager->setEnabled(settings->notificationsEnabled());
    notificationManager->setAppSettings(settings);

    // Connect bandwidth tracking
    QObject::connect(slackAPI, &SlackAPI::bandwidthBytesAdded,
                     settings, &AppSettings::addBandwidthBytes);

    // Connect polling interval settings
    slackAPI->setRefreshInterval(settings->pollingInterval());  // Initialize from settings
    QObject::connect(settings, &AppSettings::pollingIntervalChanged,
                     slackAPI, [slackAPI, settings]() {
        slackAPI->setRefreshInterval(settings->pollingInterval());
    });

    // Expose to QML
    QQmlContext *context = view->rootContext();
    context->setContextProperty("slackAPI", slackAPI);
    context->setContextProperty("workspaceManager", workspaceManager);
    context->setContextProperty("notificationManager", notificationManager);
    context->setContextProperty("fileManager", fileManager);
    context->setContextProperty("oauthManager", oauthManager);
    context->setContextProperty("statsManager", statsManager);
    context->setContextProperty("updateChecker", updateChecker);
    context->setContextProperty("draftManager", draftManager);
    context->setContextProperty("dbusAdaptor", dbusAdaptor);
    context->setContextProperty("conversationModel", conversationModel);
    context->setContextProperty("messageModel", messageModel);
    context->setContextProperty("userModel", userModel);
    context->setContextProperty("appSettings", settings);
    context->setContextProperty("appVersion", QStringLiteral(APP_VERSION));

    // Check for updates once on startup
    updateChecker->checkForUpdates();

    view->setSource(SailfishApp::pathTo("qml/harbour-lagoon.qml"));
    view->show();

    return app->exec();
}
