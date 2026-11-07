import QtQuick
import QtQuick.Controls
import SignatureMaster

// A request's state as a small coloured pill.
Rectangle {
    id: pill

    property string state: "pending"
    property string label: ""

    readonly property color tone: state === "pending" || state === "received" ? Theme.pending : state === "signing" ? Theme.signing : state === "signed" ? Theme.signed : state === "expired" ? Theme.expired : Theme.refused

    implicitWidth: text.implicitWidth + 20
    implicitHeight: 24
    radius: 12
    color: Qt.rgba(tone.r, tone.g, tone.b, Theme.dark ? 0.22 : 0.14)

    Label {
        id: text
        anchors.centerIn: parent
        text: pill.label
        font.pixelSize: 12
        font.weight: Font.DemiBold
        color: pill.tone
    }
}
