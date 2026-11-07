import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SignatureMaster

// A titled card on the Settings page. Children go into its column.
Rectangle {
    id: card

    property string title
    default property alias content: column.data

    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    implicitHeight: column.implicitHeight + 32
    radius: 8
    color: Theme.surface
    border.color: Theme.border

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        Label {
            text: card.title
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
    }
}
