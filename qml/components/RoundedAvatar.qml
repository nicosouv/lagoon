import QtQuick 2.0
import Sailfish.Silica 1.0

// Cheap avatar: no offscreen render pass (no OpacityMask/ShaderEffect layer).
// Square image with a thin circular border; initial-letter circle as fallback.
Item {
    id: root

    property alias source: image.source
    property string fallbackText: "?"
    property int fallbackFontSize: Theme.fontSizeMedium

    Image {
        id: image
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        visible: status === Image.Ready
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.rgba(Theme.highlightColor, 0.3)
        border.width: 1
        radius: width / 2
        visible: image.status === Image.Ready
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
        visible: image.status !== Image.Ready

        Label {
            anchors.centerIn: parent
            text: root.fallbackText
            font.pixelSize: root.fallbackFontSize
            color: Theme.primaryColor
        }
    }
}
