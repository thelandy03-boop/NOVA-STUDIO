import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    color: "#1E1F24"

    property real barWidth: 80.0          // Ancho exacto de cada compás (80px)
    property real currentPlayheadX: 0     // Inicia en el Compás 1 (x = 0)

    property alias scrollY: gridFlickable.contentY

    // 🧹 CONEXIÓN LIMPIA C++ PARA SINCRO DE AGUJA
    Connections {
        target: typeof AudioEngine !== "undefined" ? AudioEngine : null

        function onPositionChanged() {
            root.currentPlayheadX = AudioEngine.beatToPixel(AudioEngine.currentBeat, root.barWidth);
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
            barWidth: root.barWidth
        }

        // 2. Grilla y Playhead
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Flickable {
                id: gridFlickable
                anchors.fill: parent
                contentWidth: timelineGrid.width
                contentHeight: timelineGrid.height
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalAndVerticalFlick

                LogicTimelineGrid {
                    id: timelineGrid
                    barWidth: root.barWidth
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

            // 🎯 AGUJA DE TIEMPO INTERACTIVA
            LogicPlayhead {
                timelineX: root.currentPlayheadX
                contentX: gridFlickable.contentX
                barWidth: root.barWidth
            }
        }
    }
}