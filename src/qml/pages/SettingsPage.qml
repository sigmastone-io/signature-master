import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// One centred card: a sidebar of sections on the left, the chosen section on the right. Every
// change saves as it is made.
Item {
    id: page

    property int section: 0

    readonly property var sections: [{
            "title": "Signing",
            "icon": "icons/pen.svg",
            "blurb": "What the desk does when a request passes its checks."
        }, {
            "title": "Certificate",
            "icon": "icons/certificate.svg",
            "blurb": "The key that signs, and the timestamp that outlives it."
        }, {
            "title": "Listening",
            "icon": "icons/network.svg",
            "blurb": "Where release jobs reach this desk."
        }, {
            "title": "Who may ask",
            "icon": "icons/key.svg",
            "blurb": "The GitHub runs this desk accepts."
        }]

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 1000)
        height: parent.height - 40
        radius: 10
        color: Theme.surface
        border.color: Theme.border
        clip: true

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1 // inside the card's border
            spacing: 0

            // The sections.
            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                color: Theme.rail
                // A clip is rectangular, so the sidebar carries the card's rounding itself on the
                // left and is squared off on the right.
                radius: card.radius - 1

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.radius
                    color: parent.color
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 2

                    Label {
                        text: "Settings"
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        Layout.leftMargin: 8
                        Layout.topMargin: 6
                        Layout.bottomMargin: 14
                    }

                    Repeater {
                        model: page.sections

                        delegate: ItemDelegate {
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            text: modelData.title
                            icon.source: modelData.icon
                            icon.width: 16
                            icon.height: 16
                            spacing: 10
                            checkable: true
                            checked: page.section === index
                            highlighted: checked
                            onClicked: page.section = index
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        text: "Saved as you change them, in " + Settings.filePath
                        wrapMode: Text.WrapAnywhere
                        color: Theme.subtle
                        font.pixelSize: 11
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: Theme.border
                }
            }

            // The chosen section.
            ScrollView {
                id: content
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    x: 28
                    y: 24
                    width: content.availableWidth - 56
                    spacing: 20

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: page.sections[page.section].title
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: page.sections[page.section].blurb
                            color: Theme.muted
                            wrapMode: Text.WordWrap
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.border
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 24
                        currentIndex: page.section

                        // Signing.
                        ColumnLayout {
                            spacing: 18

                            ToggleRow {
                                title: "Automatically sign"
                                description: "Requests that pass every check, the caller's identity included, are signed without a confirmation step. A toast says so while this is on, the tray icon shows it, and each signature still goes into the log."
                                checked: Settings.autoSign
                                onToggled: checked => Settings.autoSign = checked
                            }

                            ToggleRow {
                                title: "Remember PIN"
                                description: "The certificate PIN is kept in Windows Credential Manager for this user and handed to the card on each signature. Off: the card's own dialog asks every time and the application never sees it. Arrives with the card."
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

                        // Certificate.
                        ColumnLayout {
                            spacing: 18

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: certificateColumn.implicitHeight + 28
                                radius: 8
                                color: Theme.canvas
                                border.color: Theme.border

                                ColumnLayout {
                                    id: certificateColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 14
                                    spacing: 10

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

                                    RowLayout {
                                        spacing: 8

                                        Button {
                                            text: "Choose from the user store"
                                            onClicked: Desk.chooseCertificate()
                                        }

                                        Button {
                                            text: "Make a test certificate"
                                            ToolTip.visible: hovered
                                            ToolTip.text: "A self-signed code signing certificate in your personal store, three months, for development only"
                                            onClicked: {
                                                enabled = false;
                                                testCertificateError.text = Desk.makeTestCertificate();
                                                enabled = true;
                                            }
                                        }
                                    }

                                    Label {
                                        id: testCertificateError
                                        visible: text.length > 0
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        color: Theme.refused
                                    }
                                }
                            }

                            TextRow {
                                label: "Timestamp server"
                                value: Settings.timestampUrl
                                mono: true
                                hint: "Every signature is countersigned here, so it stays valid after the certificate expires."
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

                        // Listening.
                        ColumnLayout {
                            spacing: 18

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 16

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

                        // Who may ask.
                        ColumnLayout {
                            spacing: 18

                            Label {
                                Layout.fillWidth: true
                                text: "A GitHub Actions run proves itself with its OIDC token, bound to the request it sends; the desk checks the token against GitHub's keys and these three rules. A request without a token is listed as unverified: you can still approve it by hand, it is never signed automatically."
                                wrapMode: Text.WordWrap
                                color: Theme.muted
                            }

                            TextRow {
                                label: "Allowed owner"
                                value: Settings.allowedOwner
                                mono: true
                                onCommitted: text => Settings.allowedOwner = text
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

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
                                            color: Theme.canvas
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

                                    // The field and its button wrap as one.
                                    Row {
                                        spacing: 6

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
                }
            }
        }
    }
}
