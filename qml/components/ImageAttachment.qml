import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: imageAttachment

    property string imageUrl: ""
    property string thumbUrl: ""
    property int imageWidth: 0
    property int imageHeight: 0
    property string title: ""

    width: parent.width
    height: imageLoader.height + (titleLabel.visible ? titleLabel.height : 0)

    Column {
        width: parent.width
        spacing: Theme.paddingSmall

        Label {
            id: titleLabel
            width: parent.width
            text: title
            visible: title.length > 0
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
        }

        Item {
            id: imageContainer
            width: parent.width
            height: Math.min(imageLoader.item ? imageLoader.item.sourceSize.height : 200, Screen.height / 3)

            property bool useAuthProvider: false
            property string finalImageUrl: ""

            Component.onCompleted: {
                var url = thumbUrl || imageUrl
                console.log("ImageAttachment: thumbUrl=" + thumbUrl + ", imageUrl=" + imageUrl)
                if (url && url.length > 0) {
                    // Check if URL requires Slack authentication
                    if (url.indexOf("files.slack.com") !== -1) {
                        useAuthProvider = true
                        finalImageUrl = "image://slack/" + url
                        console.log("ImageAttachment: Using auth provider: " + finalImageUrl)
                    } else {
                        useAuthProvider = false
                        finalImageUrl = url
                        console.log("ImageAttachment: Using direct URL: " + finalImageUrl)
                    }
                } else {
                    console.log("ImageAttachment: No URL available")
                }
            }

            Loader {
                id: imageLoader
                anchors.fill: parent
                sourceComponent: imageContainer.useAuthProvider ? staticImageComponent : animatedImageComponent

                Component {
                    id: staticImageComponent
                    Image {
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        smooth: true
                        source: imageContainer.finalImageUrl

                        onStatusChanged: {
                            console.log("ImageAttachment: Image status changed to " + status + " (Loading=" + Image.Loading + ", Ready=" + Image.Ready + ", Error=" + Image.Error + ")")
                            if (status === Image.Error) {
                                console.log("ImageAttachment: ERROR loading image from " + source)
                            } else if (status === Image.Ready) {
                                console.log("ImageAttachment: SUCCESS loading image from " + source)
                            }
                        }
                    }
                }

                Component {
                    id: animatedImageComponent
                    AnimatedImage {
                        fillMode: AnimatedImage.PreserveAspectFit
                        asynchronous: true
                        smooth: true
                        playing: true
                        source: imageContainer.finalImageUrl

                        onStatusChanged: {
                            console.log("ImageAttachment: AnimatedImage status changed to " + status + " (Loading=" + AnimatedImage.Loading + ", Ready=" + AnimatedImage.Ready + ", Error=" + AnimatedImage.Error + ")")
                            if (status === AnimatedImage.Error) {
                                console.log("ImageAttachment: ERROR loading animated image from " + source)
                            } else if (status === AnimatedImage.Ready) {
                                console.log("ImageAttachment: SUCCESS loading animated image from " + source)
                            }
                        }
                    }
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: imageLoader.item && imageLoader.item.status === Image.Loading
                size: BusyIndicatorSize.Medium
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("Failed to load image")
                visible: imageLoader.item && imageLoader.item.status === Image.Error
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("../pages/ImageViewerPage.qml"), {
                        "imageUrl": imageUrl,
                        "title": title
                    })
                }
            }

            // Image size indicator
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: Theme.paddingSmall
                width: sizeLabel.width + Theme.paddingMedium * 2
                height: sizeLabel.height + Theme.paddingSmall * 2
                color: Theme.rgba(Theme.highlightDimmerColor, 0.7)
                radius: Theme.paddingSmall
                visible: imageWidth > 0 && imageHeight > 0

                Label {
                    id: sizeLabel
                    anchors.centerIn: parent
                    text: imageWidth + " × " + imageHeight
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.primaryColor
                }
            }
        }
    }
}
