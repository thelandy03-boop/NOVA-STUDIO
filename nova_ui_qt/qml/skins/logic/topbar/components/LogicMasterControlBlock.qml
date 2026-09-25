import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 270
    implicitHeight: 70

    // Separador izquierdo del bloque Master
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#0E0F13"
    }

    RowLayout {
        anchors.centerIn: parent
        spacing: 20

        // Fader Master
        Column {
            spacing: 4

            Text {
                text: "Master Fader"
                color: "#A0A5B5"
                font.pixelSize: 12
                font.weight: Font.Medium
                font.family: "sans-serif"
            }

            Row {
                spacing: 8

                // Pista del Fader
                Rectangle {
                    width: 110
                    height: 8
                    radius: 4
                    color: "#14161B"
                    border.color: "#0B0C0E"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter

                    // Puntero / Knob del Fader
                    Rectangle {
                        x: 65
                        anchors.verticalCenter: parent.verticalCenter
                        width: 12
                        height: 16
                        radius: 2
                        color: "#8B909D"
                        border.color: "#181A22"
                        border.width: 1
                    }
                }

                Text {
                    text: "peak"
                    color: "#6C707C"
                    font.pixelSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Botón Save
        Rectangle {
            width: 68
            height: 36
            radius: 5
            color: "#383C46"
            border.color: "#1A1C22"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "Save"
                color: "#FFFFFF"
                font.pixelSize: 13
                font.weight: Font.Medium
            }
        }
    }
}