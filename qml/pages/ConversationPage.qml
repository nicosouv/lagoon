import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: conversationPage

    property string channelId
    property string channelName

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: channelName
                description: qsTr("%n members", "", 0)
            }

            SilicaListView {
                id: messageListView
                width: parent.width
                height: conversationPage.height - header.height - inputPanel.height

                model: messageModel
                verticalLayoutDirection: ListView.BottomToTop

                delegate: MessageDelegate { }

                ViewPlaceholder {
                    enabled: messageListView.count === 0
                    text: qsTr("No messages")
                }

                VerticalScrollDecorator { }
            }
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Channel info")
                onClicked: {
                    // TODO: Show channel info
                }
            }
            MenuItem {
                text: qsTr("Search")
                onClicked: {
                    // TODO: Implement search in conversation
                }
            }
        }
    }

    DockedPanel {
        id: inputPanel
        dock: Dock.Bottom
        width: parent.width
        height: messageInput.height + Theme.paddingLarge * 2
        open: true

        Row {
            anchors.fill: parent
            anchors.margins: Theme.paddingMedium
            spacing: Theme.paddingSmall

            IconButton {
                id: attachButton
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-attach"

                onClicked: {
                    pageStack.push(filePickerPage)
                }
            }

            TextArea {
                id: messageInput
                width: parent.width - sendButton.width - attachButton.width - parent.spacing * 2
                placeholderText: qsTr("Type a message...")
                label: qsTr("Message")

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: sendButton.clicked()
            }

            IconButton {
                id: sendButton
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-message"
                enabled: messageInput.text.length > 0

                onClicked: {
                    if (messageInput.text.trim().length > 0) {
                        slackAPI.sendMessage(channelId, messageInput.text)
                        messageInput.text = ""
                    }
                }
            }
        }
    }

    Component {
        id: filePickerPage
        Page {
            id: picker

            SilicaFlickable {
                anchors.fill: parent
                contentHeight: column.height

                Column {
                    id: column
                    width: parent.width

                    PageHeader {
                        title: qsTr("Upload File")
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Choose Image")
                        onClicked: {
                            imagePickerDialog.open()
                        }
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Choose File")
                        onClicked: {
                            filePickerDialog.open()
                        }
                    }
                }
            }

            // Image picker dialog
            Loader {
                id: imagePickerDialog
                function open() {
                    active = true
                    if (item) item.open()
                }

                active: false
                sourceComponent: Component {
                    Dialog {
                        property string selectedFile: ""

                        canAccept: selectedFile.length > 0

                        SilicaListView {
                            anchors.fill: parent
                            header: DialogHeader {
                                title: qsTr("Select Image")
                            }

                            model: Qt.application.arguments // Placeholder
                            delegate: ListItem {
                                Label {
                                    text: qsTr("Image picker not yet implemented")
                                }
                            }
                        }

                        onAccepted: {
                            if (selectedFile.length > 0) {
                                fileManager.uploadImage(channelId, selectedFile)
                                pageStack.pop()
                            }
                        }
                    }
                }
            }

            // File picker dialog (similar structure)
            Loader {
                id: filePickerDialog
                function open() {
                    active = true
                    if (item) item.open()
                }

                active: false
                sourceComponent: Component {
                    Dialog {
                        property string selectedFile: ""

                        canAccept: selectedFile.length > 0

                        SilicaListView {
                            anchors.fill: parent
                            header: DialogHeader {
                                title: qsTr("Select File")
                            }

                            model: Qt.application.arguments // Placeholder
                            delegate: ListItem {
                                Label {
                                    text: qsTr("File picker not yet implemented")
                                }
                            }
                        }

                        onAccepted: {
                            if (selectedFile.length > 0) {
                                fileManager.uploadFile(channelId, selectedFile)
                                pageStack.pop()
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: slackAPI

        onMessageReceived: {
            if (message.channel === channelId) {
                messageModel.addMessage(message)
            }
        }
    }
}
