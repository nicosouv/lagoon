import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: firstPage
    objectName: "firstPage"

    // Allow swiping right to access Stats page
    allowedOrientations: Orientation.All

    property bool isLoading: false

    // Track expanded state for sections
    property bool channelsExpanded: appSettings.channelsSectionExpanded
    property bool dmsExpanded: appSettings.dmsSectionExpanded
    property bool groupsExpanded: appSettings.groupsSectionExpanded

    Component.onCompleted: {
        console.log("FirstPage loaded - fetching conversations")
        console.log("Authenticated:", slackAPI.isAuthenticated)
        console.log("Workspace:", slackAPI.workspaceName)

        // Load conversations automatically
        loadWorkspaceData()

        // Attach StatsPage to the right
        pageStack.pushAttached(Qt.resolvedUrl("StatsPage.qml"))
    }

    // Listen for workspace switches
    Connections {
        target: workspaceManager
        onWorkspaceSwitched: {
            console.log("Workspace switched, reloading data...")
            // Clear old data
            conversationModel.clear()
            userModel.clear()
            messageModel.clear()

            // Show loading indicator and reload
            isLoading = true
            loadWorkspaceData()
        }
    }

    // Listen for conversations loaded
    Connections {
        target: slackAPI
        onConversationsReceived: {
            isLoading = false
        }
    }

    function loadWorkspaceData() {
        isLoading = true
        slackAPI.fetchConversations()
        slackAPI.fetchUsers()
    }

    // Filter conversations by type
    function filterConversationsByType(type, forceShow) {
        var result = []
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var item = conversationModel.get(i)

            // Normalize type matching
            var itemType = item.type
            var matches = false

            if (type === "channel") {
                matches = (itemType === "channel" || itemType === "group")
            } else if (type === "im") {
                matches = (itemType === "im")
            } else if (type === "mpim") {
                matches = (itemType === "mpim")
            }

            if (matches) {
                // Always show if: unread messages, starred, or section is expanded
                if (forceShow || item.unreadCount > 0 || item.isStarred) {
                    result.push(item)
                }
            }
        }
        return result
    }

    // Count conversations by type
    function countByType(type) {
        var count = 0
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var item = conversationModel.get(i)
            var itemType = item.type

            if (type === "channel" && (itemType === "channel" || itemType === "group")) {
                count++
            } else if (type === "im" && itemType === "im") {
                count++
            } else if (type === "mpim" && itemType === "mpim") {
                count++
            }
        }
        return count
    }

    // Count unread by type
    function countUnreadByType(type) {
        var count = 0
        for (var i = 0; i < conversationModel.rowCount(); i++) {
            var item = conversationModel.get(i)
            var itemType = item.type

            if (type === "channel" && (itemType === "channel" || itemType === "group")) {
                if (item.unreadCount > 0) count++
            } else if (type === "im" && itemType === "im") {
                if (item.unreadCount > 0) count++
            } else if (type === "mpim" && itemType === "mpim") {
                if (item.unreadCount > 0) count++
            }
        }
        return count
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + header.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Workspace Insights")
                onClicked: pageStack.push(Qt.resolvedUrl("StatsPage.qml"))
            }
            MenuItem {
                text: qsTr("Switch Workspace")
                visible: workspaceManager.workspaceCount() > 1
                onClicked: pageStack.push(Qt.resolvedUrl("WorkspaceSwitcher.qml"))
            }
            MenuItem {
                text: qsTr("Browse Channels")
                onClicked: pageStack.push(Qt.resolvedUrl("BrowseChannelsPage.qml"))
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"))
            }
            MenuItem {
                text: qsTr("Refresh")
                onClicked: slackAPI.fetchConversations()
            }
        }

        PageHeader {
            id: header
            title: slackAPI.workspaceName || "Lagoon"
            description: slackAPI.isAuthenticated ? qsTr("Connected") : qsTr("Disconnected")
        }

        Column {
            id: contentColumn
            anchors.top: header.bottom
            width: parent.width

            // Channels Section
            ExpandingSection {
                id: channelsSection
                width: parent.width
                title: qsTr("Channels") + " (" + countByType("channel") + ")"
                expanded: channelsExpanded

                content.sourceComponent: Column {
                    width: channelsSection.width

                    Repeater {
                        model: conversationModel

                        delegate: ChannelDelegate {
                            width: channelsSection.width

                            // Filter: only show if type matches AND (expanded OR has unreads OR starred)
                            visible: {
                                var isChannel = (type === "channel" || type === "group")
                                if (!isChannel) return false

                                // Show if section expanded OR has unread messages OR is starred
                                return channelsExpanded || unreadCount > 0 || isStarred
                            }

                            height: visible ? implicitHeight : 0

                            onClicked: {
                                console.log("Channel clicked:", name, id)

                                // Mark channel as read (clear unread count)
                                conversationModel.updateUnreadCount(id, 0)

                                // Clear notifications for this channel
                                notificationManager.clearChannelNotifications(id)

                                // Set current channel ID
                                messageModel.currentChannelId = id

                                // Fetch messages for this channel
                                slackAPI.fetchConversationHistory(id)

                                // Navigate to conversation page
                                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                                    "channelId": id,
                                    "channelName": name
                                })
                            }
                        }
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("No channels")
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        visible: countByType("channel") === 0
                        height: visible ? implicitHeight + Theme.paddingLarge * 2 : 0
                    }
                }

                onExpandedChanged: {
                    channelsExpanded = expanded
                    appSettings.channelsSectionExpanded = expanded
                }
            }

            // Direct Messages Section
            ExpandingSection {
                id: dmsSection
                width: parent.width
                title: qsTr("Direct Messages") + " (" + countByType("im") + ")"
                expanded: dmsExpanded

                content.sourceComponent: Column {
                    width: dmsSection.width

                    Repeater {
                        model: conversationModel

                        delegate: ChannelDelegate {
                            width: dmsSection.width

                            // Filter: only show if type is "im" AND (expanded OR has unreads OR starred)
                            visible: {
                                var isDM = (type === "im")
                                if (!isDM) return false

                                // Show if section expanded OR has unread messages OR is starred
                                return dmsExpanded || unreadCount > 0 || isStarred
                            }

                            height: visible ? implicitHeight : 0

                            onClicked: {
                                console.log("DM clicked:", name, id)

                                conversationModel.updateUnreadCount(id, 0)
                                notificationManager.clearChannelNotifications(id)
                                messageModel.currentChannelId = id
                                slackAPI.fetchConversationHistory(id)

                                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                                    "channelId": id,
                                    "channelName": name
                                })
                            }
                        }
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("No direct messages")
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        visible: countByType("im") === 0
                        height: visible ? implicitHeight + Theme.paddingLarge * 2 : 0
                    }
                }

                onExpandedChanged: {
                    dmsExpanded = expanded
                    appSettings.dmsSectionExpanded = expanded
                }
            }

            // Group Messages Section
            ExpandingSection {
                id: groupsSection
                width: parent.width
                title: qsTr("Group Messages") + " (" + countByType("mpim") + ")"
                expanded: groupsExpanded
                visible: countByType("mpim") > 0  // Hide section if no group messages

                content.sourceComponent: Column {
                    width: groupsSection.width

                    Repeater {
                        model: conversationModel

                        delegate: ChannelDelegate {
                            width: groupsSection.width

                            // Filter: only show if type is "mpim" AND (expanded OR has unreads OR starred)
                            visible: {
                                var isGroupMsg = (type === "mpim")
                                if (!isGroupMsg) return false

                                // Show if section expanded OR has unread messages OR is starred
                                return groupsExpanded || unreadCount > 0 || isStarred
                            }

                            height: visible ? implicitHeight : 0

                            onClicked: {
                                console.log("Group clicked:", name, id)

                                conversationModel.updateUnreadCount(id, 0)
                                notificationManager.clearChannelNotifications(id)
                                messageModel.currentChannelId = id
                                slackAPI.fetchConversationHistory(id)

                                pageStack.push(Qt.resolvedUrl("ConversationPage.qml"), {
                                    "channelId": id,
                                    "channelName": name
                                })
                            }
                        }
                    }
                }

                onExpandedChanged: {
                    groupsExpanded = expanded
                    appSettings.groupsSectionExpanded = expanded
                }
            }
        }

        ViewPlaceholder {
            enabled: conversationModel.rowCount() === 0 && !isLoading
            text: qsTr("No conversations")
            hintText: qsTr("Pull down to refresh")
        }

        VerticalScrollDecorator { }
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: isLoading
        visible: running
    }
}
