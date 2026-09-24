import QtQuick
import QtQuick.Layouts
import "../icons"

RowLayout {
    id: root

    property real volume: 0.7071 // 0.0 a 1.0
    property string dbText: "+0.0 dB"

    spacing: 8
    Layout.alignment: Qt.AlignVCenter

    // Icono Altavoz
    IconSpeaker {
        Layout.alignment: Qt.AlignVCenter
        color: "#A0A5B5"
    }

    // Área del Slider (Medidores Estéreo Paralelos + Thumb Circular)
    Item {
        id: sliderArea
        implicitWidth: 80
        implicitHeight: 28
        Layout.alignment: Qt.AlignVCenter

        // Las 2 barras horizontales paralelas del DOM de BandLab (Canal L y R)
        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            spacing: 3

            // Canal Izquierdo (L)
            Rectangle {
                width: parent.width
                height: 2
                radius: 1
                color: "#252838"
            }

            // Canal Derecho (R)
            Rectangle {
                width: parent.width
                height: 2
                radius: 1
                color: "#252838"
            }
        }

        // Handle Thumb Circular (Borde blanco + centro translúcido)
        Rectangle {
            id: thumb
            x: Math.max(0, Math.min(sliderArea.width - width, root.volume * sliderArea.width - width / 2))
            anchors.verticalCenter: parent.verticalCenter
            width: 16
            height: 16
            radius: 8
            color: "#33FFFFFF"
            border.color: "#FFFFFF"
            border.width: 2
        }

        // Interacción Drag / Click
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor

            function updateVolume(mouse) {
                var newVol = Math.max(0.0, Math.min(1.0, mouse.x / sliderArea.width));
                root.volume = newVol;
                
                // Cálculo dinámico de dB en mock (-40dB a +6dB)
                if (newVol === 0) {
                    root.dbText = "-∞ dB";
                } else {
                    var db = (newVol - 0.7071) * 20.0;
                    root.dbText = (db >= 0 ? "+" : "") + db.toFixed(1) + " dB";
                }
            }

            onPressed: (mouse) => updateVolume(mouse)
            onPositionChanged: (mouse) => updateVolume(mouse)
        }
    }

    // Texto de lectura dB
    Text {
        text: root.dbText
        color: "#FFFFFF"
        font.pixelSize: 10
        font.family: "Monospace"
        Layout.alignment: Qt.AlignVCenter
    }
}
