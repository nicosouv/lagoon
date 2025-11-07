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
        // Check if we have saved workspaces
        if (workspaceManager.workspaceCount() > 0) {
            var token = workspaceManager.currentWorkspaceToken
            if (token && token.length > 0) {
                slackAPI.authenticate(token)
                fileManager.setToken(token)
            } else {
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        } else {
            pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
        }
    }

    Connections {
        target: slackAPI

        onAuthenticationChanged: {
            if (slackAPI.isAuthenticated) {
                slackAPI.fetchConversations()
                slackAPI.fetchUsers()
                slackAPI.connectWebSocket()

                // Save workspace info after successful authentication
                workspaceManager.addWorkspace(
                    slackAPI.workspaceName,
                    workspaceManager.currentWorkspaceToken || "",
                    slackAPI.workspaceName, // teamId - to be improved with real team ID
                    slackAPI.currentUserId,
                    slackAPI.workspaceName + ".slack.com"
                )
            } else {
                pageStack.replace(Qt.resolvedUrl("pages/LoginPage.qml"))
            }
        }

        onAuthenticationError: {
            console.error("Authentication error:", error)
        }

        onNetworkError: {
            console.error("Network error:", error)
        }

        onMessageReceived: {
            // Handle real-time message
            console.log("New message received")
        }
    }
}
