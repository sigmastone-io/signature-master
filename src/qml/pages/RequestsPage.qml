import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import SignatureMaster

// The request list and the detail pane. The list is empty until the request store arrives; the
// layout and the empty states are final.
RowLayout {
    spacing: 0

    Rectangle {
        Layout.preferredWidth: 360
        Layout.fillHeight: true
        color: Theme.surfaceAlt

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Flow {
                Layout.fillWidth: true
                Layout.margins: 12
                Layout.bottomMargin: 8
                spacing: 6

                Repeater {
                    model: ["All", "Pending", "Signed", "Refused"]

                    delegate: Button {
                        required property string modelData
                        required property int index

                        text: modelData
                        checkable: true
                        checked: index === 0
                        autoExclusive: true
                        font.pixelSize: 12
                        implicitHeight: 26
                        leftPadding: 10
                        rightPadding: 10
                    }
                }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: 0

                ColumnLayout {
                    visible: list.count === 0
                    anchors.centerIn: parent
                    width: parent.width - 48
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: "No requests yet"
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: Desk.listening ? "Jobs reach this desk at " + Desk.endpoint : "The desk is not listening, so no job can reach it."
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.border
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 420)
            spacing: 8

            Item {
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 40
                implicitHeight: 40

                Image {
                    id: shield
                    anchors.fill: parent
                    source: "icons/shield.svg"
                    sourceSize: Qt.size(40, 40)
                    visible: false
                }

                MultiEffect {
                    anchors.fill: shield
                    source: shield
                    colorization: 1.0
                    colorizationColor: Theme.subtle
                    opacity: 0.6
                }
            }

            Label {
                Layout.fillWidth: true
                text: "Select a request"
                font.pixelSize: 16
                font.weight: Font.Medium
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                text: "Its facts, checks and files appear here. Signing happens on this screen and nowhere else."
                color: Theme.muted
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
