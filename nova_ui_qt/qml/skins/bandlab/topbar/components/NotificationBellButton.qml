import QtQuick
import QtQuick.Layouts
import "../icons"

Rectangle {
    id: root

    property int badgeCount: 3

    signal clicked()

    implicitWidth: 28
    implicitHeight: 28
    width: implicitWidth
    height: implicitHeight
    radius: 14
    color: "#181A24"
    border.color: "#282B3C"
    border.width: 1

    IconBell {
        anchors.centerIn: parent
        color: "#FFFFFF"
    }

    // Badge rojo de notificaciones (Esquina superior derecha)
    Rectangle {
        visible: root.badgeCount > 0
        width: 14
        height: 14
        radius: 7
        color: "#FF3B30"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: -2
        anchors.topMargin: -2

        Text {
            anchors.centerIn: parent
            text: root.badgeCount > 99 ? "99+" : root.badgeCount.toString()
            color: "#FFFFFF"
            font.pixelSize: 8
            font.bold: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
