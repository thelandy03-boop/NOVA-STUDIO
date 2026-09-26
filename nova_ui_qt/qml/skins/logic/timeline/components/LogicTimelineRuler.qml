import QtQuick

Item {
    id: root
    implicitHeight: 28

    property int barWidth: 80
    property int totalBars: 60
    property real contentX: 0

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

                    // Línea principal de compás
                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: "#353945"
                    }

                    // Número de compás
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 5
                        anchors.top: parent.top
                        anchors.topMargin: 3
                        text: (index + 1).toString()
                        color: "#9CA1B0"
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        font.family: "sans-serif"
                    }

                    // Sub-divisiones
                    Row {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 8

                        Repeater {
                            model: 4
                            Item {
                                width: root.barWidth / 4
                                height: parent.height

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    width: 1
                                    height: index === 0 ? 8 : 4
                                    color: index === 0 ? "#353945" : "#2A2D38"
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