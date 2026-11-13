// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import SignatureMaster

// The request list and the detail pane. Signing happens here and nowhere else.
RowLayout {
    id: page

    // The request under the cursor, as a map of the model's roles, or null.
    property var request: null
    // Bumped by a timer so the ages in the list keep moving.
    property int tick: 0

    spacing: 0

    function refresh() {
        const map = Requests.get(list.currentIndex);
        request = map && map.id ? map : null;
    }

    function age(date) {
        void page.tick;
        if (!date || !date.getTime || date.getTime() <= 0) {
            return "";
        }
        const seconds = Math.max(0, (Date.now() - date.getTime()) / 1000);
        if (seconds < 60) {
            return "just now";
        }
        if (seconds < 3600) {
            return Math.floor(seconds / 60) + " min ago";
        }
        if (seconds < 86400) {
            const hours = Math.floor(seconds / 3600);
            return hours + (hours === 1 ? " h ago" : " h ago");
        }
        return date.toLocaleDateString(Qt.locale(), "yyyy-MM-dd");
    }

    function fileWord(count) {
        return count === 1 ? "1 file" : count + " files";
    }

    Timer {
        interval: 30000
        running: true
        repeat: true
        onTriggered: page.tick++
    }

    Connections {
        target: Requests

        function onRowChanged(row) {
            if (row === list.currentIndex) {
                page.refresh();
            }
        }

        function onModelReset() {
            if (list.currentIndex < 0 && Requests.count > 0) {
                list.currentIndex = 0;
            }
            page.refresh();
        }
    }

    Rectangle {
        Layout.preferredWidth: 360
        Layout.fillHeight: true
        color: Theme.surfaceAlt

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 12
                Layout.bottomMargin: 8
                spacing: 6

                Repeater {
                    model: [{
                            "label": "All",
                            "key": "all"
                        }, {
                            "label": "Pending",
                            "key": "pending"
                        }, {
                            "label": "Signed",
                            "key": "signed"
                        }, {
                            "label": "Refused",
                            "key": "refused"
                        }]

                    delegate: Button {
                        required property var modelData

                        Layout.fillWidth: true
                        text: modelData.label
                        checkable: true
                        checked: Requests.filter === modelData.key
                        autoExclusive: true
                        font.pixelSize: 12
                        implicitHeight: 26
                        leftPadding: 6
                        rightPadding: 6
                        onClicked: Requests.filter = modelData.key
                    }
                }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: Requests
                currentIndex: -1
                onCurrentIndexChanged: page.refresh()

                delegate: ItemDelegate {
                    id: row

                    required property int index
                    required property string name
                    required property string refShort
                    required property string state
                    required property string stateLabel
                    required property string reason
                    required property string actor
                    required property int fileCount
                    required property var requestedAt

                    readonly property color tone: state === "pending" || state === "received" ? Theme.pending : state === "signing" ? Theme.signing : state === "signed" ? Theme.signed : state === "expired" ? Theme.expired : Theme.refused
                    readonly property bool negative: state === "refused" || state === "rejected" || state === "failed"

                    width: ListView.view.width
                    highlighted: ListView.isCurrentItem
                    onClicked: list.currentIndex = index

                    contentItem: RowLayout {
                        spacing: 10

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 5
                            width: 10
                            height: 10
                            radius: 5
                            color: row.tone
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: row.name
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: page.age(row.requestedAt)
                                    font.pixelSize: 12
                                    color: Theme.muted
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: row.refShort
                                font.family: Theme.mono
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }

                            Label {
                                Layout.fillWidth: true
                                text: row.negative ? row.stateLabel.toLowerCase() + ": " + row.reason : [page.fileWord(row.fileCount), row.actor, row.stateLabel.toLowerCase()].filter(part => part.length > 0).join(" · ")
                                font.pixelSize: 12
                                color: row.negative ? Theme.refused : Theme.muted
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                ColumnLayout {
                    visible: list.count === 0
                    anchors.centerIn: parent
                    width: parent.width - 48
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: Requests.filter === "all" ? "No requests yet" : "Nothing here"
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: Requests.filter !== "all" ? "No request is " + Requests.filter + "." : Desk.listening ? "Jobs reach this desk at " + Desk.endpoint : "The desk is not listening, so no job can reach it."
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
            visible: page.request === null
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

        ScrollView {
            id: detail
            visible: page.request !== null
            anchors.fill: parent
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                x: 24
                y: 20
                width: detail.availableWidth - 48
                spacing: 20

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            Layout.fillWidth: true
                            text: page.request ? "<b>" + page.request.name + "</b> <span style=\"font-family: '" + Theme.mono + "'; font-weight: normal\">" + page.request.refShort + "</span>" : ""
                            textFormat: Text.RichText
                            font.pixelSize: 20
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: page.request ? "Requested " + page.request.requestedAt.toLocaleString(Qt.locale(), "yyyy-MM-dd HH:mm") + (page.request.actor.length > 0 ? " by <b>" + page.request.actor + "</b>" : "") + (page.request.workflow.length > 0 || page.request.runId.length > 0 ? " from " + (page.request.runUrl.length > 0 ? "<a href=\"" + page.request.runUrl + "\">" : "") + page.request.workflow + (page.request.job.length > 0 ? " / " + page.request.job : "") + (page.request.runId.length > 0 ? ", run " + page.request.runId : "") + (page.request.runUrl.length > 0 ? "</a>" : "") : " outside GitHub Actions") : ""
                            textFormat: Text.StyledText
                            wrapMode: Text.WordWrap
                            color: Theme.muted
                            linkColor: Theme.accent
                            onLinkActivated: link => Qt.openUrlExternally(link)
                        }
                    }

                    StatePill {
                        Layout.alignment: Qt.AlignTop
                        state: page.request ? page.request.state : ""
                        label: page.request ? page.request.stateLabel : ""
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: facts.implicitHeight + 28
                    radius: 8
                    color: Theme.surface
                    border.color: Theme.border

                    GridLayout {
                        id: facts
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 14
                        columns: 2
                        columnSpacing: 24
                        rowSpacing: 12

                        Fact {
                            label: "Repository"
                            value: page.request ? page.request.repository : ""
                        }

                        Fact {
                            label: "Ref"
                            value: page.request ? page.request.ref : ""
                            mono: true
                        }

                        Fact {
                            label: "Commit"
                            value: page.request ? (page.request.commitShort.length > 0 ? page.request.commitShort : "not given") : ""
                            mono: true
                        }

                        Fact {
                            label: "Authenticated as"
                            value: page.request ? page.request.authenticatedAs : ""
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Checks"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: Theme.muted
                    }

                    Repeater {
                        model: page.request ? page.request.checks : []

                        delegate: CheckRow {
                            required property var modelData

                            outcome: modelData.outcome
                            title: modelData.title
                            detail: modelData.detail
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Files"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        color: Theme.muted
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: fileColumn.implicitHeight
                        radius: 8
                        color: Theme.surface
                        border.color: Theme.border
                        clip: true

                        ColumnLayout {
                            id: fileColumn
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: 0

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.margins: 12
                                Layout.topMargin: 10
                                Layout.bottomMargin: 10
                                spacing: 12

                                Label {
                                    Layout.fillWidth: true
                                    text: "NAME"
                                    font.pixelSize: 11
                                    font.letterSpacing: 0.5
                                    color: Theme.subtle
                                }

                                Label {
                                    Layout.preferredWidth: 80
                                    text: "SIZE"
                                    font.pixelSize: 11
                                    font.letterSpacing: 0.5
                                    color: Theme.subtle
                                }

                                Label {
                                    Layout.preferredWidth: 170
                                    text: "SHA-256"
                                    font.pixelSize: 11
                                    font.letterSpacing: 0.5
                                    color: Theme.subtle
                                }

                                Label {
                                    Layout.preferredWidth: 90
                                    text: "VERSION"
                                    font.pixelSize: 11
                                    font.letterSpacing: 0.5
                                    color: Theme.subtle
                                }

                                Label {
                                    Layout.preferredWidth: 90
                                    text: "SIGNATURE"
                                    font.pixelSize: 11
                                    font.letterSpacing: 0.5
                                    color: Theme.subtle
                                }
                            }

                            Repeater {
                                model: page.request ? page.request.files : []

                                delegate: ColumnLayout {
                                    required property var modelData

                                    Layout.fillWidth: true
                                    spacing: 0

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 1
                                        color: Theme.border
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.margins: 12
                                        Layout.topMargin: 10
                                        Layout.bottomMargin: 10
                                        spacing: 12

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.name
                                            font.family: Theme.mono
                                            font.pixelSize: 12
                                            elide: Text.ElideMiddle
                                        }

                                        Label {
                                            Layout.preferredWidth: 80
                                            text: modelData.sizeText
                                        }

                                        Label {
                                            Layout.preferredWidth: 170
                                            text: modelData.sha256Short
                                            font.family: Theme.mono
                                            font.pixelSize: 12
                                            color: Theme.muted
                                            ToolTip.visible: hashArea.containsMouse
                                            ToolTip.text: modelData.sha256

                                            MouseArea {
                                                id: hashArea
                                                anchors.fill: parent
                                                hoverEnabled: true
                                            }
                                        }

                                        Label {
                                            Layout.preferredWidth: 90
                                            text: modelData.version.length > 0 ? modelData.version : "—"
                                            color: modelData.version.length > 0 ? Theme.text : Theme.subtle
                                        }

                                        Label {
                                            Layout.preferredWidth: 90
                                            text: modelData.state === "expected" ? "waiting" : modelData.state === "received" ? "none yet" : modelData.state === "signing" ? "signing…" : modelData.state
                                            color: modelData.state === "signed" ? Theme.signed : modelData.state === "failed" ? Theme.refused : Theme.muted
                                        }
                                    }

                                    Label {
                                        visible: modelData.error.length > 0
                                        Layout.fillWidth: true
                                        Layout.leftMargin: 12
                                        Layout.rightMargin: 12
                                        Layout.bottomMargin: 10
                                        text: modelData.error
                                        color: Theme.refused
                                        font.pixelSize: 12
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.border
                }

                // Pending: reason, Reject, Sign. Anything else: what happened.
                RowLayout {
                    visible: page.request !== null && page.request.state === "pending"
                    Layout.fillWidth: true
                    Layout.bottomMargin: 20
                    spacing: 10

                    Label {
                        text: "Reason"
                        color: Theme.muted
                    }

                    TextField {
                        id: reasonField
                        Layout.fillWidth: true
                        placeholderText: "Optional, sent back to the job on reject"
                    }

                    Button {
                        text: "Reject"
                        onClicked: {
                            if (page.request && Requests.reject(page.request.id, reasonField.text)) {
                                reasonField.text = "";
                            }
                        }
                    }

                    Button {
                        text: page.request ? "Sign " + page.fileWord(page.request.fileCount) : "Sign"
                        enabled: page.request !== null && Desk.certificateFound && !Desk.certificateExpired
                        highlighted: enabled
                        ToolTip.visible: hovered && !enabled
                        ToolTip.text: "No usable certificate"
                        onClicked: {
                            if (page.request) {
                                Requests.sign(page.request.id);
                            }
                        }
                    }
                }

                // Signing: what signtool is doing right now.
                RowLayout {
                    visible: page.request !== null && page.request.state === "signing"
                    Layout.fillWidth: true
                    Layout.bottomMargin: 20
                    spacing: 10

                    BusyIndicator {
                        running: visible
                        implicitWidth: 24
                        implicitHeight: 24
                    }

                    Label {
                        Layout.fillWidth: true
                        text: page.request ? (page.request.reason.length > 0 ? page.request.reason : "Signing") + " (approved by " + page.request.approvedBy + ")" : ""
                        wrapMode: Text.WordWrap
                        color: Theme.muted
                    }
                }

                RowLayout {
                    visible: page.request !== null && page.request.state !== "pending" && page.request.state !== "signing"
                    Layout.fillWidth: true
                    Layout.bottomMargin: 20
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: page.request ? (page.request.state === "received" ? "Waiting for the job to upload " + page.fileWord(page.request.fileCount) + "." : page.request.stateLabel + (page.request.reason.length > 0 ? ": " + page.request.reason : "") + (page.request.approvedBy.length > 0 ? (page.request.state === "rejected" ? " (rejected by " : " (approved by ") + page.request.approvedBy + ")" : "") + ", " + page.request.updatedAt.toLocaleString(Qt.locale(), "yyyy-MM-dd HH:mm")) : ""
                        wrapMode: Text.WordWrap
                        color: page.request && (page.request.state === "refused" || page.request.state === "rejected" || page.request.state === "failed") ? Theme.refused : Theme.muted
                    }
                }
            }
        }
    }
}
