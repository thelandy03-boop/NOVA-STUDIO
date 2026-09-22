import QtQuick
import QtQuick.Controls
import "../../theme"

Rectangle {
    id: root
    property string title: ""
    property string subtitle: ""
    property color accent: Theme.novaIndigo
    signal clicked()

    width: 200
    height: 110
    radius: 14
    color: Theme.homeCard
    border.color: "#E2E8F0"
    border.width: 1

    Rectangle {
        width: 4
        height: parent.height
        radius: 2
        color: root.accent
        anchors.left: parent.left
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6

        Text {
            text: root.title
            color: Theme.textDark
            font.pixelSize: 15
            font.bold: true
        }
        Text {
            text: root.subtitle
            color: Theme.textMuted
            font.pixelSize: 12
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        hoverEnabled: true
        onEntered: root.border.color = root.accent
        onExited: root.border.color = "#E2E8F0"
    }
}
