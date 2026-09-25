import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    color: "#1E1F24"

    property real currentPlayheadX: 240

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