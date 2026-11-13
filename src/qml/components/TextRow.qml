// SPDX-FileCopyrightText: 2026 Sigmastone OÜ
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// A labelled text field bound to a stored value. Editing reports `committed` when the field
// loses focus or Return is pressed; the text then rebinds to the stored value, which may have
// refused the edit.
ColumnLayout {
    id: row

    property string label
    property string value
    property string hint: ""
    property bool mono: false
    property string placeholder: ""

    signal committed(string text)

    Layout.fillWidth: true
    spacing: 4

    Label {
        text: row.label
        font.weight: Font.Medium
    }

    TextField {
        id: field
        Layout.fillWidth: true
        text: row.value
        placeholderText: row.placeholder
        font.family: row.mono ? Theme.mono : Qt.application.font.family
        font.pixelSize: row.mono ? 12 : font.pixelSize
        onEditingFinished: {
            if (text !== row.value) {
                row.committed(text);
            }
            text = Qt.binding(() => row.value);
        }
    }

    Label {
        Layout.fillWidth: true
        visible: row.hint.length > 0
        text: row.hint
        wrapMode: Text.WordWrap
        color: Theme.muted
    }
}
