import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: imageViewerPage

    property string imageUrl: ""
    property string title: ""

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Save to Gallery")
                onClicked: {
                    // Download image to Pictures directory
                    var picturesPath = StandardPaths.pictures
                    var fileName = imageUrl.split('/').pop()
                    fileManager.downloadImage(imageUrl, picturesPath + "/" + fileName)
                }
            }

            MenuItem {
                text: qsTr("Share")
                onClicked: {
                    // TODO: Implement share functionality
                }
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: imageViewerPage.title || qsTr("Image")
            }

            Item {
                width: parent.width
                height: Screen.height - pageHeader.height

                PinchArea {
                    id: pinchArea
                    anchors.fill: parent

                    property real initialWidth
                    property real initialHeight

                    onPinchStarted: {
                        initialWidth = imageViewer.width
                        initialHeight = imageViewer.height
                    }

                    onPinchUpdated: {
                        var scale = pinch.scale
                        imageViewer.width = Math.max(parent.width, Math.min(parent.width * 3, initialWidth * scale))
                        imageViewer.height = Math.max(parent.height, Math.min(parent.height * 3, initialHeight * scale))
                    }

                    Flickable {
                        anchors.fill: parent
                        contentWidth: imageViewer.width
                        contentHeight: imageViewer.height
                        clip: true

                        Image {
                            id: imageViewer
                            width: parent.width
                            height: parent.height
                            source: imageUrl
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            smooth: true

                            BusyIndicator {
                                anchors.centerIn: parent
                                running: imageViewer.status === Image.Loading
                                size: BusyIndicatorSize.Large
                            }

                            Label {
                                anchors.centerIn: parent
                                text: qsTr("Failed to load image")
                                visible: imageViewer.status === Image.Error
                                color: Theme.secondaryColor
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onDoubleClicked: {
                                // Reset zoom
                                imageViewer.width = Qt.binding(function() { return parent.width })
                                imageViewer.height = Qt.binding(function() { return parent.height })
                            }
                        }
                    }
                }
            }
        }
    }
}
