import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// Four cards; every change saves as it is made.
ScrollView {
    id: page

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        x: 24
        y: 20
        width: page.availableWidth - 48
        spacing: 16

        Label {
            text: "Settings"
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 900 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 16

            SettingsCard {
                title: "Signing behaviour"

                ToggleRow {
                    title: "Automatically sign"
                    description: "Requests that pass every check are signed without a confirmation step. An orange banner stays on the window while this is on, and the tray icon shows it. Each signature still goes into the log."
                    checked: Settings.autoSign
                    onToggled: checked => Settings.autoSign = checked
                }

                ToggleRow {
                    title: "Remember PIN"
                    description: "The certificate PIN is kept in Windows Credential Manager for this user and handed to the card on each signature. Off: the card's own dialog asks every time and the application never sees it. Arrives with the signing step."
                    enabled: false
                    checked: Settings.rememberPin
                    onToggled: checked => Settings.rememberPin = checked
                }

                ToggleRow {
                    title: "Start with Windows"
                    description: "Starts in the tray when you sign in."
                    checked: Settings.startWithWindows
                    onToggled: checked => Settings.startWithWindows = checked
                }
            }

            SettingsCard {
                title: "Certificate"

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: certificateColumn.implicitHeight + 24
                    radius: 6
                    color: "transparent"
                    border.color: Theme.border

                    ColumnLayout {
                        id: certificateColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: Desk.certificateFound ? Desk.certificateSubject : "No certificate chosen"
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                visible: Desk.certificateFound && Desk.certificateSelfSigned
                                implicitWidth: pillLabel.implicitWidth + 16
                                implicitHeight: 20
                                radius: 10
                                color: Theme.bannerWarningBackground

                                Label {
                                    id: pillLabel
                                    anchors.centerIn: parent
                                    text: "self-signed, test only"
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    color: Theme.bannerWarningForeground
                                }
                            }
                        }

                        GridLayout {
                            visible: Desk.certificateFound
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 16
                            rowSpacing: 4

                            Label {
                                text: "Issuer"
                                color: Theme.muted
                            }

                            Label {
                                Layout.fillWidth: true
                                text: Desk.certificateIssuer
                                elide: Text.ElideRight
                            }

                            Label {
                                text: "Thumbprint"
                                color: Theme.muted
                            }

                            Label {
                                Layout.fillWidth: true
                                text: Desk.certificateThumbprint
                                font.family: Theme.mono
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }

                            Label {
                                text: Desk.certificateExpired ? "Expired" : "Expires"
                                color: Theme.muted
                            }

                            Label {
                                text: Desk.certificateExpires.toLocaleDateString(Qt.locale(), "yyyy-MM-dd")
                                color: Desk.certificateExpired ? Theme.refused : Theme.text
                            }
                        }

                        Label {
                            visible: !Desk.certificateFound
                            Layout.fillWidth: true
                            text: Settings.certificateThumbprint.length > 0 ? "The chosen certificate (" + Settings.certificateThumbprint + ") is no longer in your personal store." : "Pick a code signing certificate from your personal store. Until the card arrives, a self-signed one made for testing does."
                            color: Theme.muted
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                RowLayout {
                    spacing: 8

                    Button {
                        text: "Choose from the user store"
                        onClicked: Desk.chooseCertificate()
                    }

                    Button {
                        text: "Make a test certificate"
                        enabled: false
                        ToolTip.visible: hovered
                        ToolTip.text: "Comes with the signing step"
                    }
                }

                TextRow {
                    label: "Timestamp server"
                    value: Settings.timestampUrl
                    mono: true
                    onCommitted: text => Settings.timestampUrl = text
                }

                TextRow {
                    label: "signtool"
                    value: Settings.signtoolPath
                    mono: true
                    placeholder: "Not found in a Windows SDK; enter the path"
                    onCommitted: text => Settings.signtoolPath = text
                }
            }

            SettingsCard {
                title: "Listening"

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12

                    TextRow {
                        label: "Bind address"
                        value: Settings.bindAddress
                        mono: true
                        onCommitted: text => Settings.bindAddress = text
                    }

                    TextRow {
                        label: "Port"
                        value: Settings.port.toString()
                        mono: true
                        onCommitted: text => Settings.port = parseInt(text)
                    }
                }

                RowLayout {
                    spacing: 8

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: Desk.listening ? Theme.signed : (Settings.pauseListening ? Theme.expired : Theme.refused)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: Desk.listening ? "Listening. Reachable as " + Desk.endpoint + " on this network." : (Settings.pauseListening ? "Paused." : Desk.listenError)
                        color: Desk.listening ? Theme.signed : (Settings.pauseListening ? Theme.muted : Theme.refused)
                        wrapMode: Text.WordWrap
                    }
                }

                ToggleRow {
                    title: "Pause listening"
                    description: "Closes the port. Jobs fail at once instead of waiting."
                    checked: Settings.pauseListening
                    onToggled: checked => Settings.pauseListening = checked
                }

                TextRow {
                    label: "Work directory"
                    value: Settings.workDirectory
                    mono: true
                    hint: "Files of a request live here while it is open and are deleted when it ends."
                    onCommitted: text => Settings.workDirectory = text
                }
            }

            SettingsCard {
                title: "Who may ask"

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: noteLabel.implicitHeight + 20
                    radius: 6
                    color: Theme.rail

                    Label {
                        id: noteLabel
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        text: "Callers prove who they are through GitHub. The exact scheme is still to be decided; this card holds whatever it needs."
                        wrapMode: Text.WordWrap
                    }
                }

                TextRow {
                    label: "Allowed owner"
                    value: Settings.allowedOwner
                    mono: true
                    onCommitted: text => Settings.allowedOwner = text
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "Allowed repositories"
                        font.weight: Font.Medium
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: Settings.allowedRepositories

                            delegate: Rectangle {
                                id: chip

                                required property string modelData

                                implicitWidth: chipRow.implicitWidth + 20
                                implicitHeight: 26
                                radius: 13
                                color: Theme.surface
                                border.color: Theme.border

                                RowLayout {
                                    id: chipRow
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Label {
                                        text: chip.modelData
                                        font.family: Theme.mono
                                        font.pixelSize: 12
                                    }

                                    Label {
                                        text: "✕"
                                        font.pixelSize: 11
                                        color: removeArea.containsMouse ? Theme.refused : Theme.subtle

                                        MouseArea {
                                            id: removeArea
                                            anchors.fill: parent
                                            anchors.margins: -6
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: Settings.removeAllowedRepository(chip.modelData)
                                        }

                                        ToolTip.visible: removeArea.containsMouse
                                        ToolTip.text: "Remove " + chip.modelData
                                    }
                                }
                            }
                        }

                        TextField {
                            id: newRepository
                            implicitHeight: 26
                            implicitWidth: 180
                            placeholderText: "repository name"
                            font.family: Theme.mono
                            font.pixelSize: 12
                            onAccepted: {
                                Settings.addAllowedRepository(text);
                                text = "";
                            }
                        }

                        Button {
                            text: "Add"
                            implicitHeight: 26
                            font.pixelSize: 12
                            enabled: newRepository.text.trim().length > 0
                            onClicked: {
                                Settings.addAllowedRepository(newRepository.text);
                                newRepository.text = "";
                            }
                        }
                    }
                }

                TextRow {
                    label: "Allowed refs"
                    value: Settings.allowedRefs
                    mono: true
                    hint: "A glob over the full ref. Release tags are refs/tags/v*."
                    onCommitted: text => Settings.allowedRefs = text
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            text: "Settings are saved as you change them, in " + Settings.filePath
            color: Theme.muted
            font.pixelSize: 12
            elide: Text.ElideMiddle
        }
    }
}
