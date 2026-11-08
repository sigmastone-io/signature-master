import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import SignatureMaster

// One notice at the bottom of the window: a coloured mark, the message, an optional action, and
// a dismiss button. Dismissing it is remembered; the footer still tells the state.
Rectangle {
    id: toast

    property string kind: "warning" // "auto", "warning" or "error"
    property string message
    property string actionText: ""

    signal actionTriggered
    signal dismissed

    readonly property color tone: kind === "auto" ? Theme.autoSign : kind === "error" ? Theme.refused : Theme.pending

    width: Math.min(560, parent ? parent.width - 48 : 560)
    implicitHeight: row.implicitHeight + 24
    radius: 10
    color: Theme.surface
    border.color: Theme.border
    opacity: 0
    scale: 0.97

    Component.onCompleted: {
        opacity = 1;
        scale = 1;
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 160
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: 160
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 1
        width: 4
        radius: 2
        color: toast.tone
    }

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 18
        anchors.rightMargin: 8
        spacing: 10

        Item {
            implicitWidth: 16
            implicitHeight: 16
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2

            Image {
                id: glyph
                anchors.fill: parent
                source: toast.kind === "auto" ? "icons/bolt.svg" : "icons/warning.svg"
                sourceSize: Qt.size(16, 16)
                visible: false
            }

            MultiEffect {
                anchors.fill: glyph
                source: glyph
                colorization: 1.0
                colorizationColor: toast.tone
            }
        }

        Label {
            Layout.fillWidth: true
            text: toast.message
            textFormat: Text.StyledText
            wrapMode: Text.WordWrap
        }

        Button {
            visible: toast.actionText.length > 0
            flat: true
            text: toast.actionText
            font.weight: Font.Medium
            onClicked: toast.actionTriggered()
        }

        ToolButton {
            text: "✕"
            font.pixelSize: 12
            ToolTip.visible: hovered
            ToolTip.text: "Dismiss; the footer keeps saying it"
            onClicked: toast.dismissed()
        }
    }
}
