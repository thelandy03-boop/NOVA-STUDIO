import QtQuick

Item {
    id: root

    implicitWidth: 16
    implicitHeight: 16
    width: implicitWidth
    height: implicitHeight

    property color color: "#A0A5B5"
    property alias iconColor: root.color

    Image {
        anchors.centerIn: parent
        width: root.width
        height: root.height
        source: Qt.resolvedUrl("svg/IconBell.svg")
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
    }
}
