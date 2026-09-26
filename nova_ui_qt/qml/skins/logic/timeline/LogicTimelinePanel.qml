import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    color: "#1E1F24"

    property real barWidth: 80.0          // Ancho exacto de cada compás (80px)
    property real sampleRate: 44100.0     // Frecuencia de muestreo
    property real currentPlayheadX: 0     // Inicia en el Compás 1 (x = 0)

    // CONEXIÓN DIRECTA CON C++ PARA SINCRO CON GRID Y REGLA
    Connections {
        target: typeof AudioEngine !== "undefined" ? AudioEngine : null

        function onPositionChanged() {
            var bpm = AudioEngine.bpm > 0 ? AudioEngine.bpm : 120.0;
            var currentSeconds = AudioEngine.currentFrame / root.sampleRate;
            
            // Convertir segundos -> beats -> píxeles del grid
            var currentBeats = currentSeconds * (bpm / 60.0);
            var pixelsPerBeat = root.barWidth / 4.0; // 4/4
            
            root.currentPlayheadX = currentBeats * pixelsPerBeat;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Regla
        LogicTimelineRuler {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            contentX: gridFlickable.contentX
        }

        // 2. Grilla y Playhead
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Flickable {
                id: gridFlickable
                anchors.fill: parent
                contentWidth: 4800
                contentHeight: 960
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalAndVerticalFlick

                LogicTimelineGrid {
                    width: 4800
                    height: 960
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton

                    onWheel: (wheel) => {
                        if (wheel.modifiers & Qt.ShiftModifier) {
                            var newY = gridFlickable.contentY - wheel.angleDelta.y
                            gridFlickable.contentY = Math.max(0, Math.min(gridFlickable.contentHeight - gridFlickable.height, newY))
                        } else {
                            var delta = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.angleDelta.x
                            var newX = gridFlickable.contentX - delta
                            gridFlickable.contentX = Math.max(0, Math.min(gridFlickable.contentWidth - gridFlickable.width, newX))
                        }
                    }
                }
            }

            LogicPlayhead {
                timelineX: root.currentPlayheadX
                contentX: gridFlickable.contentX
            }
        }
    }
}