import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: emojiPicker

    property string selectedEmoji: ""

    canAccept: selectedEmoji.length > 0

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: qsTr("Pick an emoji")
            }

            // Common emojis
            SectionHeader {
                text: qsTr("Frequently used")
            }

            Flow {
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingSmall

                Repeater {
                    model: ["👍", "❤️", "😂", "🎉", "👏", "🔥", "✅", "👀"]

                    BackgroundItem {
                        width: Theme.itemSizeSmall
                        height: Theme.itemSizeSmall

                        Label {
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: Theme.fontSizeLarge
                        }

                        onClicked: {
                            selectedEmoji = modelData
                            accept()
                        }
                    }
                }
            }
        }
    }
}
