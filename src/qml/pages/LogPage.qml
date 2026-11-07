import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// The append-only signature log. The table arrives with the signing step; the frame, filters and
// empty state are final.
Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        anchors.topMargin: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: "Signature log"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: "Every signature, refusal and rejection, appended and never edited. This is what you read if the certificate ever has to be revoked."
                    color: Theme.muted
                    wrapMode: Text.WordWrap
                }
            }

            Button {
                text: "Export CSV"
                enabled: false
                ToolTip.visible: hovered
                ToolTip.text: "Nothing to export yet"
            }

            Button {
                text: "Open folder"
                onClicked: Desk.openLogFolder()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                Layout.fillWidth: true
                placeholderText: "Search repository, tag, file or hash"
                enabled: false
            }

            Repeater {
                model: ["All", "Signed", "Refused", "Rejected", "Automatic"]

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

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Theme.surface
            border.color: Theme.border

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 420)
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: "No entries yet"
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    text: "The first signature, refusal or rejection opens the log."
                    color: Theme.muted
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Label {
                text: "Log file: " + Desk.logFilePath
                color: Theme.muted
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            Label {
                text: Desk.certificateSummary
                color: Theme.muted
                elide: Text.ElideRight
            }
        }
    }
}
