import QtQuick

ReaperBaseButton {
    id: root

    implicitWidth: 22
    implicitHeight: 18

    property string label: ""
    property color labelColor: "#B0B0B0"
    property color activeColor: "#4CAF50"
    signal tapped()

    onToggled: root.tapped()

    Rectangle {
        anchors.fill: parent
        radius: 2
        color: root.active ? root.activeColor : "#262626"
        border.color: "#111111"
        border.width: 1

        // Bisel 3D interior
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 1
            color: "transparent"
            border.color: "#3F3F3F"
            border.width: 1
        }

        Text {
            anchors.centerIn: parent
            text: root.label
            color: root.active ? "#FFFFFF" : root.labelColor
            font.pixelSize: 10
            font.bold: true
        }
    }
}