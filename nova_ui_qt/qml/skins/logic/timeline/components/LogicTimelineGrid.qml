import QtQuick

Item {
    id: root
    property int barWidth: 80
    property int totalBars: 60
    property int trackHeight: 74

    // Ancho total y alto calculado dinámicamente según el número de pistas
    width: barWidth * totalBars
    height: Math.max(1080, (AudioEngine.tracks ? AudioEngine.tracks.rowCount() * trackHeight : 0) + 200)

    // Fondo base
    Rectangle {
        anchors.fill: parent
        color: "#1C1D22"
    }

    // 1. CARRILES HORIZONTALES DINÁMICOS (Sincronizados con Ardour)
    Column {
        id: lanesColumn
        width: parent.width

        Repeater {
            model: AudioEngine.tracks
            delegate: LogicTrackLane {
                width: root.width
            }
        }
    }

    // 2. LÍNEAS HORIZONTALES GUÍA SI NO HAY PISTAS O PARA EL ESPACIO VACÍO
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: lanesColumn.bottom
        anchors.bottom: parent.bottom

        Repeater {
            model: 20 // Fondo de grilla vacío estilo Logic
            Rectangle {
                width: parent.width
                height: root.trackHeight
                color: (index % 2 === 0) ? "#1A1B20" : "#17181C"
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#111216"
                }
            }
        }
    }

    // 3. CUADRÍCULA VERTICAL (Compases y Sub-divisiones / Beats)
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
                    color: "#353945"
                    opacity: 0.4
                }

                // Sub-divisiones (Beats 2, 3, 4)
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
                                color: "#252833"
                                opacity: index === 0 ? 0.0 : 0.25
                            }
                        }
                    }
                }
            }
        }
    }
}