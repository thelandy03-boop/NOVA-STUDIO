import QtQuick
import QtQuick.Layouts
import NovaStudio 1.0

Rectangle {
    id: root
    width: 170
    height: 32
    radius: 4
    color: "#18191C"
    border.color: "#2C2D32"
    border.width: 1

    property real peakL: AudioEngine.masterPeakLeft
    property real peakR: AudioEngine.masterPeakRight
    property bool clipL: peakL >= 0.98
    property bool clipR: peakR >= 0.98

    RowLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 6

        // 🎛️ ETIQUETA MASTER Y CLIP LED
        ColumnLayout {
            spacing: 2
            Layout.alignment: Qt.AlignVCenter

            Text {
                text: "MASTER"
                color: "#8E8F96"
                font.pixelSize: 8
                font.bold: true
            }

            // Indicador LED de Clip Rojo
            Rectangle {
                width: 10
                height: 10
                radius: 2
                color: (root.clipL || root.clipR) ? "#FF2222" : "#321111"
                border.color: (root.clipL || root.clipR) ? "#FF6666" : "#442222"

                Behavior on color { ColorAnimation { duration: 100 } }
            }
        }

        // 📊 BARRAS ESTÉREO (L / R) CON COLORES VERDE -> AMARILLO -> ROJO
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 3

            // CANAL L (IZQUIERDO)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#101114"
                radius: 2
                clip: true

                Rectangle {
                    width: parent.width * Math.min(1.0, root.peakL)
                    height: parent.height
                    radius: 2

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#00E676" } // Verde
                        GradientStop { position: 0.7; color: "#FFEA00" } // Amarillo
                        GradientStop { position: 0.95; color: "#FF1744" } // Rojo
                    }
                }
            }

            // CANAL R (DERECHO)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#101114"
                radius: 2
                clip: true

                Rectangle {
                    width: parent.width * Math.min(1.0, root.peakR)
                    height: parent.height
                    radius: 2

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#00E676" } // Verde
                        GradientStop { position: 0.7; color: "#FFEA00" } // Amarillo
                        GradientStop { position: 0.95; color: "#FF1744" } // Rojo
                    }
                }
            }
        }

        // 🎚️ VALOR EN dB Y CONTROL MOUSE FADER
        Rectangle {
            width: 38
            height: parent.height
            color: "#101114"
            radius: 2
            border.color: "#222328"

            Text {
                anchors.centerIn: parent
                text: {
                    var db = AudioEngine.masterVolumeDb
                    return db <= -60 ? "-∞ dB" : db.toFixed(1) + " dB"
                }
                color: "#D0D2DC"
                font.pixelSize: 9
                font.bold: true
            }

            MouseArea {
                id: masterDrag
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                property real lastY: 0

                onPressed: (mouse) => lastY = mouse.y
                onPositionChanged: (mouse) => {
                    var deltaY = lastY - mouse.y
                    lastY = mouse.y
                    var currentDb = AudioEngine.masterVolumeDb
                    var newDb = Math.max(-60.0, Math.min(6.0, currentDb + deltaY * 0.4))
                    AudioEngine.setMasterVolumeDb(newDb)
                }

                onDoubleClicked: AudioEngine.setMasterVolumeDb(0.0) // Reset a 0.0 dB
            }
        }
    }
}
