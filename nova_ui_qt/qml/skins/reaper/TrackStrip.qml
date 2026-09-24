import QtQuick
import QtQuick.Controls
import "controls"

Rectangle {
    id: root
    property string trackNumber: "3"
    property bool isSelected: true
    property real faderValue: 0.65
    property bool recArmed: true
    property bool muted: false
    property bool solo: false
    property bool fxActive: true

    width: 84
    color: root.isSelected ? "#3A3A3A" : "#1E1E1E"

    Column {
        anchors.fill: parent
        spacing: 0

        // ──────────────────────────────────────────────
        // 1. CABECERA (Pan Knob Centrado)
        // ──────────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 30
            color: root.isSelected ? "#B5B5B5" : "#8A8A8A"

            ReaperKnob {
                anchors.centerIn: parent
                value: 0.5
            }
        }

        // ──────────────────────────────────────────────
        // 2. CUERPO CANAL (Fader + Botones)
        // ──────────────────────────────────────────────
        Item {
            width: parent.width
            height: parent.height - 50

            Text {
                text: "-inf"
                color: "#7A7A7A"
                font.pixelSize: 9
                font.family: "Segoe UI"
                x: 4; y: 4
            }

            // Riel negro vertical
            Rectangle {
                width: 2
                color: "#000000"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: fader.top
                anchors.bottom: fader.bottom
                z: 0
            }

            // Fader Metálico
            ReaperFader {
                id: fader
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 16
                anchors.bottom: bottomArea.top
                width: 22
                value: root.faderValue
                onMoved: (v) => root.faderValue = v
                z: 1
            }

            // Columna Derecha de Botones
            Column {
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top: parent.top
                anchors.topMargin: 4
                spacing: 3
                width: 22

                ReaperTinyBtn {
                    label: "M"; labelColor: "#E55B5B"; activeColor: "#C62828"
                    active: root.muted; onTapped: root.muted = !root.muted
                }
                ReaperTinyBtn {
                    label: "S"; labelColor: "#FFD54F"; activeColor: "#F9A825"
                    active: root.solo; onTapped: root.solo = !root.solo
                }
                ReaperRouteBtn {}
                
                ReaperFxBtn {
                    fxActive: root.fxActive
                    onPowerToggled: root.fxActive = fxActive
                }

                ReaperNodesBtn {}
            }

            // Zona Inferior (Speaker + REC Centrado)
            Item {
                id: bottomArea
                height: 28
                width: parent.width
                anchors.bottom: parent.bottom

                ReaperSpeakerIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                }

                ReaperTrackRecButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    armed: root.recArmed
                    onToggled: root.recArmed = !root.recArmed
                }
            }
        }

        // ──────────────────────────────────────────────
        // 3. BADGE INFERIOR (Corte limpio plano)
        // ──────────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 20
            color: root.isSelected ? "#D6D6D6" : "#A6A6A6"

            // Bisel brillante superior de la placa
            Rectangle {
                width: parent.width; height: 1
                color: root.isSelected ? "#FFFFFF" : "#C8C8C8"
                anchors.top: parent.top
            }

            // ✂️ MUESCA / RECORTE SEMICIRCULAR PLANO (Sin sombras)
            Rectangle {
                width: 6
                height: 6
                radius: 3
                y: -3 // La mitad sobresale arriba creando la media luna plana
                color: root.isSelected ? "#3A3A3A" : "#1E1E1E" // Mismo color exacto del cuerpo
                anchors.horizontalCenter: parent.horizontalCenter
                antialiasing: true
            }

            // Número de pista
            Text {
                text: root.trackNumber
                color: "#000000"
                font.pixelSize: 11
                font.bold: true
                font.family: "Segoe UI"
                anchors.centerIn: parent
                anchors.verticalCenterOffset: 1
            }
        }
    }
}