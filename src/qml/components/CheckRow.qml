import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// One line of the checks list: a mark, the check's title, and its detail when there is one.
RowLayout {
    id: row

    property string outcome: "pending" // passed, failed, unverified, pending
    property string title
    property string detail: ""

    readonly property color tone: outcome === "passed" ? Theme.signed : outcome === "failed" ? Theme.refused : outcome === "unverified" ? Theme.pending : Theme.subtle

    Layout.fillWidth: true
    spacing: 8

    Rectangle {
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 2
        width: 16
        height: 16
        radius: 8
        color: Qt.rgba(row.tone.r, row.tone.g, row.tone.b, 0.18)

        Label {
            anchors.centerIn: parent
            text: row.outcome === "passed" ? "✓" : row.outcome === "failed" ? "✕" : row.outcome === "unverified" ? "?" : "…"
            font.pixelSize: 11
            font.weight: Font.Bold
            color: row.tone
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 1

        Label {
            Layout.fillWidth: true
            text: row.title
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            visible: row.detail.length > 0
            text: row.detail
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            color: row.outcome === "failed" ? Theme.refused : Theme.muted
        }
    }
}
