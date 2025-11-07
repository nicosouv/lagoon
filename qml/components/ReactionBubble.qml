import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: reactionBubble

    property string emoji: ""
    property int count: 0
    property bool isOwnReaction: false

    width: row.width + Theme.paddingMedium * 2
    height: Theme.itemSizeExtraSmall

    Rectangle {
        anchors.fill: parent
        radius: Theme.paddingSmall
        color: isOwnReaction ?
               Theme.rgba(Theme.highlightBackgroundColor, 0.3) :
               Theme.rgba(Theme.highlightBackgroundColor, 0.1)
        border.color: isOwnReaction ?
                     Theme.highlightColor :
                     Theme.rgba(Theme.highlightColor, 0.3)
        border.width: 1
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Theme.paddingSmall

        Label {
            text: emoji
            font.pixelSize: Theme.fontSizeSmall
        }

        Label {
            text: count
            font.pixelSize: Theme.fontSizeExtraSmall
            color: isOwnReaction ? Theme.highlightColor : Theme.secondaryColor
        }
    }

    onClicked: {
        // TODO: Toggle reaction
    }
}
