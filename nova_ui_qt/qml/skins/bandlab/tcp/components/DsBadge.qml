import QtQuick

Rectangle {
    id: root

    property string text: "IA"
    property color badgeColor: "#1F6BFF"
    property color textColor: "#FFFFFF"

    implicitWidth: badgeText.implicitWidth + 10
    implicitHeight: 16
    radius: 4
    color: badgeColor

    Text {
        id: badgeText
        anchors.centerIn: parent
        text: root.text
        color: root.textColor
        font.pixelSize: 9
        font.bold: true
    }
}