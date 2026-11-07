import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import SignatureMaster

// A persistent band above the content. Not dismissable: it goes when its state goes.
Rectangle {
    id: banner

    property string kind: "warning" // "auto", "warning" or "error"
    property string message
    property string actionText: ""

    signal actionTriggered

    readonly property color foreground: kind === "auto" ? Theme.bannerAutoForeground : kind === "error" ? Theme.bannerErrorForeground : Theme.bannerWarningForeground

    Layout.fillWidth: true
    implicitHeight: row.implicitHeight + 16
    color: kind === "auto" ? Theme.bannerAutoBackground : kind === "error" ? Theme.bannerErrorBackground : Theme.bannerWarningBackground

    // The wash sits on the canvas, never on another wash.
    Rectangle {
        anchors.fill: parent
        z: -1
        color: Theme.canvas
    }

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10

        Item {
            implicitWidth: 16
            implicitHeight: 16

            Image {
                id: glyph
                anchors.fill: parent
                source: banner.kind === "auto" ? "icons/bolt.svg" : "icons/warning.svg"
                sourceSize: Qt.size(16, 16)
                visible: false
            }

            MultiEffect {
                anchors.fill: glyph
                source: glyph
                colorization: 1.0
                colorizationColor: banner.foreground
            }
        }

        Label {
            Layout.fillWidth: true
            text: banner.message
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
            color: banner.foreground
        }

        Button {
            visible: banner.actionText.length > 0
            flat: true
            text: banner.actionText
            font.weight: Font.Medium
            palette.buttonText: banner.foreground
            onClicked: banner.actionTriggered()
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: banner.foreground
        opacity: 0.25
    }
}
