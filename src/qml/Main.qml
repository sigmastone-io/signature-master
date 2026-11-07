import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

ApplicationWindow {
    id: root

    property int section: 0

    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    visible: false
    title: "Signature Master"
    color: Theme.canvas
    palette.accent: Theme.accent
    palette.highlight: Theme.accent
    palette.window: Theme.canvas
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.text: Theme.text
    palette.button: Theme.surface0
    palette.buttonText: Theme.text
    palette.placeholderText: Theme.subtle
    palette.link: Theme.accent

    // Closing hides to the tray; Quit lives in the tray menu.
    onClosing: close => {
        close.accepted = false;
        root.hide();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Banner {
            visible: Settings.autoSign
            kind: "auto"
            message: "<b>Automatic signing is on.</b> Requests that pass every check are signed without confirmation. Each one is still written to the log."
            actionText: "Turn off"
            onActionTriggered: Settings.autoSign = false
        }

        Banner {
            visible: Desk.certificateFound && Desk.certificateExpired
            kind: "error"
            message: "<b>The certificate has expired.</b> Requests are refused until another one is chosen."
            actionText: "Choose"
            onActionTriggered: root.section = 2
        }

        Banner {
            visible: Desk.certificateFound && Desk.certificateSelfSigned && !Desk.certificateExpired
            kind: "warning"
            message: "<b>Test certificate in use.</b> Signatures made with " + Desk.certificateSubject + " are not trusted by anyone and must not be published."
        }

        Banner {
            visible: !Desk.certificateFound
            kind: "warning"
            message: "<b>No certificate chosen.</b> Requests are refused until one is."
            actionText: "Choose"
            onActionTriggered: root.section = 2
        }

        Banner {
            visible: Desk.listenError.length > 0
            kind: "error"
            message: "<b>Not listening.</b> " + Desk.listenError
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                color: Theme.rail

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 2

                    RailItem {
                        text: "Requests"
                        icon.source: "icons/requests.svg"
                        badge: Desk.pendingCount
                        checked: root.section === 0
                        onClicked: root.section = 0
                    }

                    RailItem {
                        text: "Log"
                        icon.source: "icons/log.svg"
                        checked: root.section === 1
                        onClicked: root.section = 1
                    }

                    RailItem {
                        text: "Settings"
                        icon.source: "icons/settings.svg"
                        checked: root.section === 2
                        onClicked: root.section = 2
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: statusColumn.implicitHeight + 20
                        radius: 6
                        color: Theme.surfaceAlt
                        border.color: Theme.border

                        ColumnLayout {
                            id: statusColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                spacing: 8

                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: Desk.listening ? Theme.signed : (Settings.pauseListening ? Theme.expired : Theme.refused)
                                }

                                Label {
                                    text: Desk.listening ? "Listening" : (Settings.pauseListening ? "Paused" : "Not listening")
                                    font.weight: Font.Medium
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: Desk.endpoint
                                font.family: Theme.mono
                                font.pixelSize: 12
                                color: Theme.muted
                                elide: Text.ElideMiddle
                            }

                            Label {
                                Layout.fillWidth: true
                                text: Desk.lastRequestAt.getTime() > 0 ? "Last request " + Desk.lastRequestAt.toLocaleTimeString(Qt.locale(), Locale.ShortFormat) : "No requests yet"
                                color: Theme.muted
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
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

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.section

                RequestsPage {
                }

                LogPage {
                }

                SettingsPage {
                }
            }
        }

        StatusFooter {
        }
    }
}
