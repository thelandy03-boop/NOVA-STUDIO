import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    implicitWidth: 260
    color: "#282A2E"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Cabecera (+ Add Track)
        LogicTcpHeader {
            Layout.fillWidth: true
        }

        // 2. Lista de Pistas Dinámica (Sincronizada con Ardour Core)
        Flickable {
            id: flickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: flickable.width
            contentHeight: trackList.height
            clip: true

            Column {
                id: trackList
                width: flickable.width
                spacing: 0

                Repeater {
                    model: AudioEngine.tracks
                    delegate: LogicTrackCard {
                        width: trackList.width
                        height: 74
                    }
                }
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