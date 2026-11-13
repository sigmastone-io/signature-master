// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

ApplicationWindow {
    id: root

    property int section: 0

    // The notices worth a toast, minus the ones dismissed. Each key names the state it describes,
    // so a dismissed notice comes back only when that state goes away and returns.
    readonly property var notices: {
        void Settings.dismissedNotices;
        const list = [];
        if (Settings.autoSign) {
            list.push({
                    "key": "auto-sign",
                    "kind": "auto",
                    "message": "<b>Automatic signing is on.</b> Requests that pass every check are signed without confirmation. Each one is still written to the log.",
                    "action": "Turn off"
                });
        }
        if (Desk.certificateFound && Desk.certificateExpired) {
            list.push({
                    "key": "certificate-expired:" + Desk.certificateThumbprint,
                    "kind": "error",
                    "message": "<b>The certificate has expired.</b> Requests are refused until another one is chosen.",
                    "action": "Choose"
                });
        }
        if (Desk.certificateFound && Desk.certificateSelfSigned && !Desk.certificateExpired) {
            list.push({
                    "key": "test-certificate:" + Desk.certificateThumbprint,
                    "kind": "warning",
                    "message": "<b>Test certificate in use.</b> Signatures made with " + Desk.certificateSubject + " are not trusted by anyone and must not be published.",
                    "action": ""
                });
        }
        if (!Desk.certificateFound) {
            list.push({
                    "key": "no-certificate",
                    "kind": "warning",
                    "message": "<b>No certificate chosen.</b> Requests are refused until one is.",
                    "action": "Choose"
                });
        }
        if (Desk.listenError.length > 0) {
            list.push({
                    "key": "not-listening:" + Desk.listenError,
                    "kind": "error",
                    "message": "<b>Not listening.</b> " + Desk.listenError,
                    "action": ""
                });
        }
        return list.filter(notice => !Settings.isDismissed(notice.key));
    }

    function act(key) {
        if (key === "auto-sign") {
            Settings.autoSign = false;
        } else {
            root.section = 2;
        }
    }

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

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                StackLayout {
                    anchors.fill: parent
                    currentIndex: root.section

                    RequestsPage {
                    }

                    LogPage {
                    }

                    SettingsPage {
                    }
                }

                // The toasts, bottom centre over the content, newest at the bottom.
                ColumnLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 16
                    width: Math.min(560, parent.width - 48)
                    spacing: 8
                    z: 10

                    Repeater {
                        model: root.notices

                        delegate: Toast {
                            required property var modelData

                            Layout.fillWidth: true
                            kind: modelData.kind
                            message: modelData.message
                            actionText: modelData.action
                            onActionTriggered: root.act(modelData.key)
                            onDismissed: Settings.dismissNotice(modelData.key)
                        }
                    }
                }
            }
        }

        StatusFooter {
        }
    }
}
