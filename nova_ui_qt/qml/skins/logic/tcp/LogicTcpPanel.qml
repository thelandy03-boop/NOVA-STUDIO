import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    implicitWidth: 260
    color: "#282A2E" // Fondo oscuro para contrastar con las tarjetas grises

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Cabecera (+ Add Track)
        LogicTcpHeader {
            Layout.fillWidth: true
        }

        // 2. Lista de Pistas (Estilo REAPER)
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: trackList.height
            clip: true

            Column {
                id: trackList
                width: parent.width
                spacing: 0 // Sin espacio, pegadas una con otra como en REAPER

                LogicTrackCard { trackNum: "1"; trackName: "MUSIC_Full" }
                LogicTrackCard { trackNum: "2"; trackName: "Synth Lead A" }
                LogicTrackCard { trackNum: "3"; trackName: "Kick" }
                LogicTrackCard { trackNum: "4"; trackName: "Snare" }
                LogicTrackCard { trackNum: "5"; trackName: "VOCALS" }
                LogicTrackCard { trackNum: "6"; trackName: "Bass" }
            }
        }
    }

    // Separador vertical derecho
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#111111"
    }
}