import QtQuick 2.0
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0

ListItem {
    id: messageItem
    contentHeight: messageColumn.height + Theme.paddingMedium * 2

    Component {
        id: imageAttachmentComponent
        ImageAttachment {
            imageUrl: attachmentData.image_url || ""
            thumbUrl: attachmentData.thumb_url || ""
            imageWidth: attachmentData.image_width || 0
            imageHeight: attachmentData.image_height || 0
            title: attachmentData.title || ""
        }
    }

    Component {
        id: fileAttachmentComponent
        FileAttachment {
            fileId: attachmentData.id || ""
            fileName: attachmentData.name || attachmentData.title || ""
            fileType: attachmentData.mimetype || ""
            fileSize: attachmentData.size || 0
            downloadUrl: attachmentData.url_private || ""
        }
    }

    Row {
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium
        spacing: Theme.paddingMedium

        // User avatar
        Item {
            id: avatarContainer
            width: Theme.iconSizeMedium
            height: Theme.iconSizeMedium

            Image {
                id: avatarImage
                anchors.fill: parent
                source: userModel.getUserAvatar(model.userId) || ""
                fillMode: Image.PreserveAspectCrop
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: avatarImage.width
                        height: avatarImage.height
                        radius: width / 2
                    }
                }
                visible: status === Image.Ready
            }

            // Fallback placeholder if image fails to load
            Rectangle {
                id: avatarPlaceholder
                anchors.fill: parent
                radius: width / 2
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.2)
                visible: avatarImage.status !== Image.Ready

                Label {
                    anchors.centerIn: parent
                    text: userModel.getUserName(model.userId).charAt(0).toUpperCase()
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.primaryColor
                }
            }
        }

        Column {
            id: messageColumn
            width: parent.width - avatarContainer.width - parent.spacing * 2
            spacing: Theme.paddingSmall

            Row {
                width: parent.width
                spacing: Theme.paddingMedium

                Label {
                    text: userModel.getUserName(model.userId) || model.userId
                    font.bold: true
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                Label {
                    text: Qt.formatDateTime(new Date(parseFloat(model.timestamp) * 1000), "hh:mm")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }

                Label {
                    text: qsTr("edited")
                    visible: model.isEdited
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryHighlightColor
                }
            }

            Label {
                width: parent.width
                text: model.text
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                textFormat: Text.PlainText
            }

            // Image attachments
            Repeater {
                model: {
                    try {
                        var attachments = messageItem.model.attachments
                        if (typeof attachments === 'string') {
                            return JSON.parse(attachments)
                        }
                        return attachments
                    } catch (e) {
                        return []
                    }
                }

                delegate: Loader {
                    width: messageColumn.width
                    sourceComponent: {
                        var attachment = modelData
                        if (attachment.image_url || attachment.thumb_url) {
                            return imageAttachmentComponent
                        } else if (attachment.url) {
                            return fileAttachmentComponent
                        }
                        return null
                    }

                    property var attachmentData: modelData
                }
            }

            // Thread indicator
            BackgroundItem {
                width: parent.width
                height: Theme.itemSizeExtraSmall
                visible: model.threadCount > 0

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("%n replies", "", model.threadCount)
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                }

                onClicked: {
                    // TODO: Open thread view
                }
            }
        }
    }

    menu: ContextMenu {
        MenuItem {
            text: qsTr("Reply in thread")
            onClicked: {
                // TODO: Reply in thread
            }
        }

        MenuItem {
            text: qsTr("Add reaction")
            onClicked: {
                // TODO: Show emoji picker
            }
        }

        MenuItem {
            text: qsTr("Copy text")
            onClicked: {
                Clipboard.text = model.text
            }
        }

        MenuItem {
            text: qsTr("Edit")
            visible: model.isOwnMessage
            onClicked: {
                // TODO: Edit message
            }
        }

        MenuItem {
            text: qsTr("Delete")
            visible: model.isOwnMessage
            onClicked: {
                remorseAction(qsTr("Deleting"), function() {
                    slackAPI.deleteMessage(model.channelId, model.timestamp)
                })
            }
        }
    }
}
