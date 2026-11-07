import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// A checkbox with a title and a description. The checked state follows `checked`; a click
// reports `toggled` and the binding is restored, so the stored value stays the truth.
RowLayout {
    id: row

    property string title
    property string description
    property bool checked: false

    signal toggled(bool checked)

    Layout.fillWidth: true
    spacing: 12

    CheckBox {
        id: box
        Layout.alignment: Qt.AlignTop
        checked: row.checked
        onToggled: {
            row.toggled(checked);
            checked = Qt.binding(() => row.checked);
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            text: row.title
            font.weight: Font.Medium
            opacity: row.enabled ? 1 : 0.6

            MouseArea {
                anchors.fill: parent
                enabled: row.enabled
                onClicked: box.toggle()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: row.description.length > 0
            text: row.description
            wrapMode: Text.WordWrap
            color: Theme.muted
            opacity: row.enabled ? 1 : 0.6
        }
    }
}
