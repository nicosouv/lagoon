import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: fileAttachment

    property string fileId: ""
    property string fileName: ""
    property string fileType: ""
    property int fileSize: 0
    property string downloadUrl: ""

    width: parent.width
    height: Theme.itemSizeSmall

    Row {
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        spacing: Theme.paddingMedium

        Icon {
            id: fileIcon
            anchors.verticalCenter: parent.verticalCenter
            source: getFileIcon(fileType)
            width: Theme.iconSizeMedium
            height: Theme.iconSizeMedium
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - fileIcon.width - downloadIcon.width - Theme.paddingMedium * 3
            spacing: Theme.paddingSmall

            Label {
                width: parent.width
                text: fileName
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeSmall
                color: fileAttachment.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Label {
                text: formatFileSize(fileSize)
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        Icon {
            id: downloadIcon
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-cloud-download"
            width: Theme.iconSizeSmall
            height: Theme.iconSizeSmall
        }
    }

    onClicked: {
        if (downloadUrl) {
            fileManager.downloadFile(fileId, downloadUrl, "")
        }
    }

    function getFileIcon(type) {
        if (type.startsWith("image/")) {
            return "image://theme/icon-m-file-image"
        } else if (type.startsWith("video/")) {
            return "image://theme/icon-m-file-video"
        } else if (type.startsWith("audio/")) {
            return "image://theme/icon-m-file-audio"
        } else if (type === "application/pdf") {
            return "image://theme/icon-m-file-pdf"
        } else if (type.includes("document") || type.includes("text")) {
            return "image://theme/icon-m-file-document"
        } else {
            return "image://theme/icon-m-file-other"
        }
    }

    function formatFileSize(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        } else if (bytes < 1024 * 1024) {
            return (bytes / 1024).toFixed(1) + " KB"
        } else if (bytes < 1024 * 1024 * 1024) {
            return (bytes / (1024 * 1024)).toFixed(1) + " MB"
        } else {
            return (bytes / (1024 * 1024 * 1024)).toFixed(1) + " GB"
        }
    }
}
