import QtQuick
import QtQuick.Layouts
import "../icons"

Rectangle {
    id: root

    property string label: "Masterizando"
    property bool active: false

    signal clicked()

    implicitWidth: contentRow.implicitWidth + 20
    implicitHeight: 28
    height: 28
    radius: 14
    color: "#141620"
    border.color: "#222534"
    border.width: 1

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6

        IconEqualizer {
            anchors.verticalCenter: parent.verticalCenter
            color: root.active ? "#FFFFFF" : "#A0A5B5"
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.label + " ›"
            color: "#FFFFFF"
            font.pixelSize: 11
            font.bold: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
