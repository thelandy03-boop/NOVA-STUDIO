import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    implicitWidth: 130
    implicitHeight: 70
    color: "#15171C"

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#0E0F13"
    }

    Column {
        anchors.centerIn: parent
        spacing: 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "NOVA"
            color: "#FFFFFF"
            font.pixelSize: 21
            font.weight: Font.DemiBold
            font.letterSpacing: 2.5
            font.family: "sans-serif"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "STUDIO"
            color: "#9A9EA8"
            font.pixelSize: 10
            font.weight: Font.Normal
            font.letterSpacing: 3.5
            font.family: "sans-serif"
        }
    }
}