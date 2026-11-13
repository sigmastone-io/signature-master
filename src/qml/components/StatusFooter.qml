// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// The state of the machine, on every section: certificate, timestamp server, automatic signing,
// PIN handling.
Rectangle {
    Layout.fillWidth: true
    implicitHeight: 28
    color: Theme.rail

    Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        Label {
            Layout.maximumWidth: parent.width * 0.5
            text: Desk.certificateSummary
            color: Desk.certificateFound && !Desk.certificateExpired ? Theme.muted : Theme.refused
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Label {
            text: "Timestamp: " + Settings.timestampUrl
            color: Theme.muted
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            text: Settings.autoSign ? "Automatic signing on" : "Automatic signing off"
            color: Settings.autoSign ? Theme.autoSign : Theme.muted
            font.pixelSize: 12
            font.weight: Settings.autoSign ? Font.Medium : Font.Normal
        }

        Label {
            text: Settings.rememberPin ? "PIN remembered" : "PIN asked by the card"
            color: Settings.rememberPin ? Theme.autoSign : Theme.muted
            font.pixelSize: 12
            font.weight: Settings.rememberPin ? Font.Medium : Font.Normal
        }
    }
}
