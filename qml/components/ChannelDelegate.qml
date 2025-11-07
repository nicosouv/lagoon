import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: channelItem
    contentHeight: Theme.itemSizeMedium

    Row {
        anchors.fill: parent
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin
        spacing: Theme.paddingMedium

        // Unread indicator
        Rectangle {
            width: Theme.paddingSmall
            height: parent.height
            color: model.unreadCount > 0 ? Theme.highlightColor : "transparent"
            visible: model.unreadCount > 0
        }

        Column {
            width: parent.width - unreadBadge.width - Theme.paddingLarge * 3
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                text: "#" + model.name
                font.bold: model.unreadCount > 0
                truncationMode: TruncationMode.Fade
                color: channelItem.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                width: parent.width
                text: model.topic || model.lastMessage || qsTr("No messages")
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        Label {
            id: unreadBadge
            anchors.verticalCenter: parent.verticalCenter
            text: model.unreadCount > 0 ? model.unreadCount : ""
            font.pixelSize: Theme.fontSizeExtraSmall
            font.bold: true
            color: Theme.highlightColor
            visible: text.length > 0
        }
    }

    menu: ContextMenu {
        MenuItem {
            text: qsTr("Mark as read")
            onClicked: {
                // TODO: Implement mark as read
            }
        }
        MenuItem {
            text: qsTr("Leave channel")
            visible: model.isMember
            onClicked: {
                remorseAction(qsTr("Leaving channel"), function() {
                    slackAPI.leaveConversation(model.id)
                })
            }
        }
    }
}
