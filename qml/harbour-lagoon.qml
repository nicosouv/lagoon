import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: appWindow

    initialPage: Component {
        FirstPage { }
    }

    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    allowedOrientations: defaultAllowedOrientations

    Component.onCompleted: {
        console.log("[App] Startup, workspaces:", workspaceManager.workspaceCount())

        if (workspaceManager.workspaceCount() > 0) {
            var token = workspaceManager.currentWorkspaceToken
            if (token && token.length > 0) {
                console.log("[App] Auto-login with saved token")
                slackAPI.authenticate(token)
                fileManager.setToken(token)
            } else {
                console.log("[App] No valid token, showing login")
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        } else {
            console.log("[App] No workspaces, showing login")
            pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
    }

    Connections {
        target: workspaceManager

        onWorkspaceSwitched: {
            console.log("[App] Workspace switched to index:", index)
            conversationModel.clear()
            messageModel.clear()
            userModel.clear()
            slackAPI.disconnectWebSocket()
            slackAPI.authenticate(token)
            fileManager.setToken(token)
        }
    }

    // Helper function to navigate to a channel
    function openChannel(channelId) {
        // Find channel info from conversationModel
        var channelName = ""
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var idx = conversationModel.index(i, 0)
            if (conversationModel.data(idx, conversationModel.IdRole) === channelId) {
                channelName = conversationModel.data(idx, conversationModel.NameRole)
                break
            }
        }

        if (channelName) {
            console.log("[App] Opening channel:", channelName)
            conversationModel.updateUnreadCount(channelId, 0)
            notificationManager.clearChannelNotifications(channelId)
            messageModel.currentChannelId = channelId
            slackAPI.fetchConversationHistory(channelId)
            pageStack.clear()
            pageStack.push(Qt.resolvedUrl("pages/FirstPage.qml"))
            pageStack.push(Qt.resolvedUrl("pages/ConversationPage.qml"), {
                "channelId": channelId,
                "channelName": channelName
            })
        } else {
            console.warn("[App] Channel not found:", channelId)
        }
    }

    Connections {
        target: notificationManager
        onNotificationClicked: openChannel(channelId)
    }

    Connections {
        target: dbusAdaptor
        onPleaseOpenChannel: {
            appWindow.activate()
            openChannel(channelId)
        }
    }

    Connections {
        target: slackAPI

        onAuthenticationChanged: {
            if (slackAPI.isAuthenticated) {
                console.log("[App] Auth success:", slackAPI.workspaceName)
                slackAPI.fetchConversations()
                slackAPI.fetchUsers()
                slackAPI.connectWebSocket()
            } else {
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        }

        onTeamIdChanged: {
            if (slackAPI.teamId && slackAPI.teamId.length > 0) {
                console.log("[App] Saving workspace:", slackAPI.workspaceName)
                workspaceManager.addWorkspace(
                    slackAPI.workspaceName,
                    slackAPI.token,
                    slackAPI.teamId,
                    slackAPI.currentUserId,
                    slackAPI.workspaceName + ".slack.com"
                )
            }
        }

        onAuthenticationError: {
            console.error("[App] Auth error:", error)
        }

        onNetworkError: {
            console.error("[App] Network error:", error)
        }

        onMessageReceived: {
            if (message.type === "message" && message.channel) {
                if (messageModel.currentChannelId === message.channel) {
                    messageModel.addMessage(message)
                }
            }
        }
    }
}
