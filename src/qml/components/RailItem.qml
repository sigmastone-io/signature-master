import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One entry of the left rail, with an optional count badge.
ItemDelegate {
    id: control

    property int badge: 0

    Layout.fillWidth: true
    checkable: true
    highlighted: checked
    spacing: 10
    icon.width: 16
    icon.height: 16
    rightPadding: badge > 0 ? badgeBox.width + 20 : 12

    Rectangle {
        id: badgeBox
        visible: control.badge > 0
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(18, badgeLabel.implicitWidth + 10)
        height: 18
        radius: 9
        color: "#d97706"

        Label {
            id: badgeLabel
            anchors.centerIn: parent
            text: control.badge > 99 ? "99+" : control.badge
            color: "#ffffff"
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }
}
