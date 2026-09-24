import QtQuick
import QtQuick.Layouts
import "../icons"

Rectangle {
    id: root

    property bool canUndo: false
    property bool canRedo: false

    signal undoClicked()
    signal redoClicked()

    implicitWidth: 60
    implicitHeight: 28
    radius: 14
    color: "#1A1C24" // Fondo sólido
    border.width: 0  // SIN borde exterior para eliminar marcos indeseados
    clip: true

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // --- BOTÓN IZQUIERDO: DESHACER ---
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                color: uHit.pressed ? "#282C3C" : (uHit.containsMouse ? "#222532" : "transparent")
            }

            IconUndo {
                anchors.centerIn: parent
                width: 14; height: 14
                iconColor: root.canUndo ? (uHit.containsMouse ? "#FFFFFF" : "#C0C4D0") : "#4A4D5C"
                opacity: root.canUndo ? 1.0 : 0.45
            }

            MouseArea {
                id: uHit
                anchors.fill: parent
                enabled: root.canUndo
                hoverEnabled: true
                cursorShape: root.canUndo ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.undoClicked()
            }
        }

        // --- LÍNEA DIVISORIA EDGE-TO-EDGE ---
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true // Corta la cápsula completamente de arriba a abajo
            color: "#090A0E"
        }

        // --- BOTÓN DERECHO: REHACER ---
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                color: rHit.pressed ? "#282C3C" : (rHit.containsMouse ? "#222532" : "transparent")
            }

            IconRedo {
                anchors.centerIn: parent
                width: 14; height: 14
                iconColor: root.canRedo ? (rHit.containsMouse ? "#FFFFFF" : "#C0C4D0") : "#4A4D5C"
                opacity: root.canRedo ? 1.0 : 0.45
            }

            MouseArea {
                id: rHit
                anchors.fill: parent
                enabled: root.canRedo
                hoverEnabled: true
                cursorShape: root.canRedo ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.redoClicked()
            }
        }
    }
}