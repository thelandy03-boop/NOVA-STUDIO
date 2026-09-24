import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import "../icons"

Rectangle {
    id: root

    property bool isPlaying: false
    property bool isRecording: false
    property bool isLooping: false
    property string timeText: "00:00.0"

    signal playClicked()
    signal rewindClicked()
    signal recordClicked()
    signal loopClicked()

    implicitWidth: transportRow.implicitWidth
    implicitHeight: 28
    height: 28
    radius: 14
    color: "#141620"
    border.color: "#222534"
    border.width: 1

    RowLayout {
        id: transportRow
        anchors.fill: parent
        spacing: 0

        // Celda 1: Play ▶
        Item {
            implicitWidth: 32
            implicitHeight: 28

            Shape {
                anchors.centerIn: parent
                width: 10
                height: 12
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    fillColor: root.isPlaying ? "#00E676" : "#FFFFFF"
                    strokeWidth: 0
                    PathSvg { path: "M 1 1 L 10 6 L 1 11 Z" }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.playClicked()
            }
        }

        // Línea divisora 1 (de borde a borde)
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 2: Rewind ⏮
        Item {
            implicitWidth: 32
            implicitHeight: 28

            Shape {
                anchors.centerIn: parent
                width: 11
                height: 11
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    fillColor: "#FFFFFF"
                    strokeWidth: 0
                    PathSvg { path: "M 1 1 H 2.5 V 10 H 1 Z M 3.5 5.5 L 10 1 V 10 Z" }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.rewindClicked()
            }
        }

        // Línea divisora 2
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 3: Record ●
        Item {
            implicitWidth: 32
            implicitHeight: 28

            Rectangle {
                anchors.centerIn: parent
                width: 10
                height: 10
                radius: 5
                color: root.isRecording ? "#FF3B30" : "#C0392B"
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.recordClicked()
            }
        }

        // Línea divisora 3
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 4: Loop 🔁
        Item {
            implicitWidth: 32
            implicitHeight: 28

            IconLoop {
                anchors.centerIn: parent
                color: root.isLooping ? "#00E676" : "#FFFFFF"
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.loopClicked()
            }
        }

        // Línea divisora 4
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 5: Timer (00:00.0)
        Item {
            implicitWidth: timeTextItem.implicitWidth + 24
            implicitHeight: 28

            Text {
                id: timeTextItem
                anchors.centerIn: parent
                text: root.timeText
                color: "#FFFFFF"
                font.pixelSize: 11
                font.bold: true
                font.family: "Monospace"
            }
        }
    }
}
