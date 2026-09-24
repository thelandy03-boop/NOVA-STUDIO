import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string text: "Button"
    property alias iconSource: iconLoader.sourceComponent

    signal clicked()

    implicitWidth: contentRow.implicitWidth + 24
    implicitHeight: 32
    radius: 16

    color: hit.pressed ? "#161722" : (hit.containsMouse ? "#222534" : "#181A24")
    border.color: hit.containsMouse ? "#323648" : "#222432"
    border.width: 1

    Behavior on color { ColorAnimation { duration: 100 } }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6

        Loader {
            id: iconLoader
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.text
            color: "#FFFFFF"
            font.pixelSize: 12
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: hit
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}