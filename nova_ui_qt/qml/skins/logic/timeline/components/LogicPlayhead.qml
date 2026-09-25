import QtQuick

Item {
    id: root

    property real timelineX: 180       // Posición absoluta en la canción (px)
    property real contentX: 0          // <--- DECLARACIÓN DE LA PROPIEDAD NECESARIA

    x: Math.round(timelineX - root.contentX) - 8
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: 16
    z: 30

    // 1. Aguja
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 13
        anchors.bottom: parent.bottom
        width: 1
        color: "#E2E6EF"
        opacity: 0.90
    }

    // 2. Cabezal SVG
    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 2
        width: 14
        height: 14
        source: Qt.resolvedUrl("../icons/svg/LogicPlayheadHead.svg")
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
    }
}