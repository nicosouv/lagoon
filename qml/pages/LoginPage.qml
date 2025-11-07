import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: loginPage

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Login to Slack")
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("To get started, you'll need a Slack OAuth token")
                wrapMode: Text.WordWrap
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeMedium
                horizontalAlignment: Text.AlignHCenter
            }

            TextField {
                id: tokenField
                width: parent.width
                placeholderText: qsTr("xoxp-...")
                label: qsTr("User OAuth Token")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: loginButton.clicked()
            }

            Button {
                id: loginButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Connect")
                preferredWidth: Theme.buttonWidthLarge
                enabled: tokenField.text.length > 0

                onClicked: {
                    if (tokenField.text.trim().length > 0) {
                        var token = tokenField.text.trim()
                        slackAPI.authenticate(token)
                        fileManager.setToken(token)

                        // Temporarily store token for workspace creation
                        workspaceManager.currentWorkspaceToken = token
                    }
                }
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("How to get your token:")
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.highlightColor
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - Theme.horizontalPageMargin * 2
                text: qsTr("1. Go to api.slack.com/apps\n" +
                          "2. Click 'Create New App'\n" +
                          "3. Choose 'From scratch'\n" +
                          "4. Name it 'SlackShip' and select your workspace\n" +
                          "5. Go to 'OAuth & Permissions'\n" +
                          "6. Add these User Token Scopes:\n" +
                          "   • channels:*, groups:*, im:*, mpim:*\n" +
                          "   • chat:write, users:read, reactions:*\n" +
                          "   • files:*, search:read\n" +
                          "7. Click 'Install to Workspace'\n" +
                          "8. Copy the 'User OAuth Token' (starts with xoxp-)")
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }
    }

    // Connection success handler
    Connections {
        target: slackAPI

        onAuthenticationSucceeded: {
            console.log("Authentication succeeded!")
            pageStack.pop()
        }

        onAuthenticationFailed: {
            console.error("Authentication failed:", errorMessage)
        }
    }
}
