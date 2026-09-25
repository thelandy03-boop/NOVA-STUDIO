import QtQuick

Item {
    id: root
    property int barWidth: 80
    property int totalBars: 60
    property int trackHeight: 60
    property int totalTracks: 16

    // Fondo base
    Rectangle {
        anchors.fill: parent
        color: "#1E1F24"
    }

    // Carriles horizontales
    Column {
        anchors.fill: parent

        Repeater {
            model: root.totalTracks

            Rectangle {
                width: parent.width
                height: root.trackHeight
                color: index % 2 === 0 ? "#1E1F24" : "#1B1C21"

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#14151A"
                }
            }
        }
    }

    // Cuadrícula vertical (Compases y Beats)
    Row {
        anchors.fill: parent

        Repeater {
            model: root.totalBars

            Item {
                width: root.barWidth
                height: parent.height

                // Línea de compás principal
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: "#282B36"
                }

                // Líneas de sub-divisiones (beats)
                Row {
                    anchors.fill: parent

                    Repeater {
                        model: 4
                        Item {
                            width: root.barWidth / 4
                            height: parent.height

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 1
                                color: "#22242D"
                                opacity: index === 0 ? 0.0 : 0.35
                            }
                        }
                    }
                }
            }
        }
    }
}