import QtQuick
import QtQuick.Layouts
import "../rack"

Rectangle {
    id: root

    property int trackIndex: index !== undefined ? index + 1 : 1
    property string trackName: "01 Voz/Audio"
    property bool isSelected: false
    property bool isMuted: false
    property bool isSoloed: false
    property real volumeValue: 0.75
    property real panValue: 0.0

    signal trackSelected(int index)

    implicitWidth: parent ? parent.width : 220
    implicitHeight: 82
    color: isSelected ? "#1A1C24" : (itemHit.containsMouse ? "#14161E" : "#0E0F13")
    border.color: "#1A1C23"
    border.width: 1

    // Indicador amarillo de la pista seleccionada (Borde izquierdo)
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: root.isSelected ? "#FFD54F" : "transparent"
    }

    MouseArea {
        id: itemHit
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.trackSelected(root.trackIndex - 1)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 6

        // FILA 1: Icono + Nombre + Menú : + Mute + Solo
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TrackIcon {
                iconType: "mic"
                iconColor: "#FFD54F"
            }

            Text {
                Layout.fillWidth: true
                text: root.trackName
                color: root.isSelected ? "#FFFFFF" : "#A0A5B5"
                font.pixelSize: 11
                font.bold: root.isSelected
                elide: Text.ElideRight
            }

            // Opciones :
            Text {
                text: "⋮"
                color: "#6E7280"
                font.pixelSize: 12
            }

            // Botón M (Mute)
            Rectangle {
                width: 18; height: 18; radius: 3
                color: root.isMuted ? "#E5A93C" : "#222532"
                Text { anchors.centerIn: parent; text: "M"; font.pixelSize: 9; font.bold: true; color: root.isMuted ? "#000" : "#8F94A0" }
                MouseArea { anchors.fill: parent; onClicked: root.isMuted = !root.isMuted }
            }

            // Botón S (Solo)
            Rectangle {
                width: 18; height: 18; radius: 3
                color: root.isSoloed ? "#00E676" : "#222532"
                Text { anchors.centerIn: parent; text: "S"; font.pixelSize: 9; font.bold: true; color: root.isSoloed ? "#000" : "#8F94A0" }
                MouseArea { anchors.fill: parent; onClicked: root.isSoloed = !root.isSoloed }
            }

            // Flecha desplegar
            Text {
                text: "˅"
                color: "#6E7280"
                font.pixelSize: 10
            }
        }

        // FILA 2: Botón + Fx
        Rectangle {
            implicitWidth: fxTxt.implicitWidth + 12
            implicitHeight: 18
            radius: 9
            color: "#1E212B"
            border.color: "#2C3040"

            Text {
                id: fxTxt
                anchors.centerIn: parent
                text: "+ Fx"
                color: "#8F94A0"
                font.pixelSize: 9
                font.bold: true
            }
        }

        // FILA 3: Slider Volumen + Perilla Pan
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Slider Fader Amarillo
            Item {
                Layout.fillWidth: true
                height: 14

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: 3
                    radius: 1.5
                    color: "#252834"

                    Rectangle {
                        width: parent.width * root.volumeValue
                        height: parent.height
                        radius: 1.5
                        color: "#FFD54F"
                    }

                    Rectangle {
                        x: Math.max(0, Math.min(parent.width - 10, parent.width * root.volumeValue - 5))
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10; height: 10; radius: 5
                        color: "#FFFFFF"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onPositionChanged: (mouse) => {
                        if (pressed) root.volumeValue = Math.max(0, Math.min(1, mouse.x / width))
                    }
                }
            }

            // Perilla Pan (Knob L/R)
            Rectangle {
                width: 16; height: 16; radius: 8
                color: "#1E212B"
                border.color: "#3A3F52"

                Rectangle {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 2
                    width: 2; height: 5
                    color: "#FFD54F"
                }
            }
        }
    }
}