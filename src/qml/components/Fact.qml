import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// A small caption over a value, for the facts of a request or a certificate.
ColumnLayout {
    id: fact

    property string label
    property string value
    property bool mono: false

    Layout.fillWidth: true
    spacing: 2

    Label {
        text: fact.label.toUpperCase()
        font.pixelSize: 11
        font.letterSpacing: 0.5
        color: Theme.subtle
    }

    Label {
        Layout.fillWidth: true
        text: fact.value
        font.family: fact.mono ? Theme.mono : Qt.application.font.family
        font.pixelSize: fact.mono ? 12 : font.pixelSize
        elide: Text.ElideMiddle
    }
}
