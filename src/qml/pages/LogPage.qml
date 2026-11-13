// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import SignatureMaster

// The append-only signature log: every signature, refusal, rejection, expiry and failure.
Item {
    id: page

    function outcomeColor(outcome) {
        return outcome === "signed" ? Theme.signed : outcome === "expired" ? Theme.expired : Theme.refused;
    }

    FileDialog {
        id: exportDialog
        title: "Export the signature log"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "csv"
        nameFilters: ["CSV (*.csv)"]
        onAccepted: exportError.text = LogEntries.exportCsv(selectedFile)
    }

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
                enabled: LogEntries.count > 0
                ToolTip.visible: hovered && !enabled
                ToolTip.text: "Nothing to export yet"
                onClicked: exportDialog.open()
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
                text: LogEntries.search
                onTextEdited: LogEntries.search = text
            }

            Repeater {
                model: [{
                        "label": "All",
                        "key": "all"
                    }, {
                        "label": "Signed",
                        "key": "signed"
                    }, {
                        "label": "Refused",
                        "key": "refused"
                    }, {
                        "label": "Rejected",
                        "key": "rejected"
                    }, {
                        "label": "Automatic",
                        "key": "automatic"
                    }]

                delegate: Button {
                    required property var modelData

                    text: modelData.label
                    checkable: true
                    checked: LogEntries.filter === modelData.key
                    autoExclusive: true
                    font.pixelSize: 12
                    implicitHeight: 26
                    leftPadding: 10
                    rightPadding: 10
                    onClicked: LogEntries.filter = modelData.key
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Theme.surface
            border.color: Theme.border
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 12
                    Layout.topMargin: 10
                    Layout.bottomMargin: 10
                    spacing: 12

                    Label {
                        Layout.preferredWidth: 140
                        text: "TIME"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }

                    Label {
                        Layout.preferredWidth: 150
                        text: "REPOSITORY"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }

                    Label {
                        Layout.preferredWidth: 120
                        text: "REF"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "FILE"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }

                    Label {
                        Layout.preferredWidth: 150
                        text: "SHA-256"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }

                    Label {
                        Layout.preferredWidth: 200
                        text: "OUTCOME"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }

                    Label {
                        Layout.preferredWidth: 90
                        text: "BY"
                        font.pixelSize: 11
                        font.letterSpacing: 0.5
                        color: Theme.subtle
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                }

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: LogEntries
                    ScrollBar.vertical: ScrollBar {
                    }

                    delegate: Item {
                        id: row

                        required property int index
                        required property var time
                        required property string name
                        required property string refShort
                        required property string file
                        required property string sha256
                        required property string sha256Short
                        required property string outcome
                        required property string by
                        required property string reason

                        width: ListView.view.width
                        implicitHeight: rowLayout.implicitHeight + 18

                        Rectangle {
                            anchors.fill: parent
                            color: row.index % 2 === 1 ? Theme.canvas : "transparent"
                            opacity: 0.5
                        }

                        RowLayout {
                            id: rowLayout
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12

                            Label {
                                Layout.preferredWidth: 140
                                text: row.time.toLocaleString(Qt.locale(), "yyyy-MM-dd HH:mm:ss")
                                font.family: Theme.mono
                                font.pixelSize: 12
                            }

                            Label {
                                Layout.preferredWidth: 150
                                text: row.name
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.preferredWidth: 120
                                text: row.refShort
                                font.family: Theme.mono
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }

                            Label {
                                Layout.fillWidth: true
                                text: row.file.length > 0 ? row.file : "no file"
                                font.family: Theme.mono
                                font.pixelSize: 12
                                color: row.file.length > 0 ? Theme.text : Theme.subtle
                                elide: Text.ElideMiddle
                            }

                            Label {
                                Layout.preferredWidth: 150
                                text: row.sha256Short
                                font.family: Theme.mono
                                font.pixelSize: 12
                                color: Theme.muted
                                ToolTip.visible: hashArea.containsMouse && row.sha256.length > 0
                                ToolTip.text: row.sha256

                                MouseArea {
                                    id: hashArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }

                            Label {
                                Layout.preferredWidth: 200
                                text: row.reason.length > 0 ? "<b>" + row.outcome + "</b> <font color=\"" + Theme.muted + "\">" + row.reason + "</font>" : "<b>" + row.outcome + "</b>"
                                textFormat: Text.StyledText
                                font.pixelSize: 12
                                color: page.outcomeColor(row.outcome)
                                elide: Text.ElideRight
                                ToolTip.visible: reasonArea.containsMouse && row.reason.length > 0
                                ToolTip.text: row.reason

                                MouseArea {
                                    id: reasonArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }

                            Label {
                                Layout.preferredWidth: 90
                                text: row.by
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }
                    }

                    ColumnLayout {
                        visible: list.count === 0
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 48, 420)
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            text: LogEntries.total === 0 ? "No entries yet" : "Nothing matches"
                            font.weight: Font.Medium
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Label {
                            Layout.fillWidth: true
                            text: LogEntries.total === 0 ? "The first signature, refusal or rejection opens the log." : "Clear the search or pick another filter."
                            color: Theme.muted
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Label {
                text: LogEntries.count === LogEntries.total ? LogEntries.total + (LogEntries.total === 1 ? " entry" : " entries") : LogEntries.count + " of " + LogEntries.total + " entries"
                color: Theme.muted
            }

            Label {
                id: exportError
                visible: text.length > 0
                color: Theme.refused
                elide: Text.ElideRight
            }

            Label {
                text: "Log file: " + LogEntries.path
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
