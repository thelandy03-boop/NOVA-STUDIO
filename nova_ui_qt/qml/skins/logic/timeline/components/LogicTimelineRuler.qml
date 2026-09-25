import QtQuick

Item {
    id: root
    implicitHeight: 28

    property int barWidth: 80
    property int totalBars: 60
    property real contentX: 0          // <--- DECLARACIÓN DE LA PROPIEDAD NECESARIA

    // Fondo
    Rectangle {
        anchors.fill: parent
        color: "#16181D"
    }

    // Regla scrolleable
    Item {
        anchors.fill: parent
        clip: true

        Row {
            x: -root.contentX
            height: parent.height

            Repeater {
                model: root.totalBars

                Item {
                    width: root.barWidth
                    height: root.height

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: "#2C303A"
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.top: parent.top
                        anchors.topMargin: 4
                        text: (index + 1).toString()
                        color: "#8F93A0"
                        font.pixelSize: 11
                        font.family: "sans-serif"
                    }

                    Row {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 6

                        Repeater {
                            model: 4
                            Item {
                                width: root.barWidth / 4
                                height: parent.height

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    width: 1
                                    height: index === 0 ? 6 : 3
                                    color: "#2C303A"
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#0E0F13"
    }
}