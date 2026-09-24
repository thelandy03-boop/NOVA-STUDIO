import QtQuick

Item {
    id: root
    property real value: 0.65
    signal moved(real val)

    width: 22

    // 🛤️ Riel nítido centrado (Sin escalones ni cortes)
    Rectangle {
        id: rail
        width: 2
        color: "#000000"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        // Micro bisel brillante a la derecha para profundidad 3D
        Rectangle {
            width: 1
            height: parent.height
            color: "#333333"
            anchors.left: parent.right
        }
    }

    // 🎚️ Thumb Metálico 3D (Centrado en el riel)
    ReaperMetalThumb {
        id: thumb
        anchors.horizontalCenter: parent.horizontalCenter
        y: (1.0 - root.value) * Math.max(0, parent.height - height)
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.SizeVerCursor
        property real lastY: 0

        onPressed: function(mouse) {
            lastY = mouse.y
            var usable = Math.max(1, root.height - thumb.height)
            root.value = Math.max(0, Math.min(1, 1.0 - (mouse.y - thumb.height / 2) / usable))
            root.moved(root.value)
        }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            var usable = Math.max(1, root.height - thumb.height)
            var delta = (lastY - mouse.y) / usable
            lastY = mouse.y
            root.value = Math.max(0, Math.min(1, root.value + delta))
            root.moved(root.value)
        }
    }
}