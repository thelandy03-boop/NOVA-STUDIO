import QtQuick

Rectangle {
    id: root

    property alias iconSource: iconLoader.sourceComponent
    property color iconColor: "#8F94A0"
    property color hoverColor: "#222534"
    property color activeColor: "#181A24"
    property bool isCircle: true

    signal clicked()

    implicitWidth: 32
    implicitHeight: 32
    radius: isCircle ? width / 2 : 6

    color: hit.pressed ? activeColor : (hit.containsMouse ? hoverColor : "transparent")
    border.color: hit.containsMouse ? "#323648" : "transparent"
    border.width: 1

    Behavior on color { ColorAnimation { duration: 100 } }

    Loader {
        id: iconLoader
        anchors.centerIn: parent
    }

    MouseArea {
        id: hit
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}