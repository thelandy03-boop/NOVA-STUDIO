import QtQuick

Item {
    id: root
    property real value: 0.5 // Rango: 0.0 (Izq) a 1.0 (Der)
    signal moved(real val)

    width: 22
    height: 22

    // ── CAPA 1: Zócalo de Profundidad ──
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#181818"
        border.color: "#3A3A3A"
        border.width: 1
    }

    // ── CAPA 2: Domo Metálico 3D Especular ──
    Rectangle {
        id: cap
        anchors.fill: parent
        anchors.margins: 1
        radius: width / 2
        border.color: "#222222"
        border.width: 1

        gradient: Gradient {
            GradientStop { position: 0.00; color: "#FFFFFF" }
            GradientStop { position: 0.25; color: "#E0E0E0" }
            GradientStop { position: 0.65; color: "#A0A0A0" }
            GradientStop { position: 1.00; color: "#555555" }
        }

        // Bisel de brillo interior
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: width / 2
            color: "transparent"
            border.color: "#FFFFFF"
            border.width: 1
            opacity: 0.6
        }

        // Punto Indicador rotatorio (Arco REAPER 270°)
        Item {
            anchors.fill: parent
            rotation: (root.value - 0.5) * 270

            Rectangle {
                width: 3
                height: 3
                radius: 1.5
                color: "#000000"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 3
            }
        }
    }

    // Arrastre con el ratón
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.SizeVerCursor
        property real lastY: 0

        onPressed: (mouse) => lastY = mouse.y
        onPositionChanged: (mouse) => {
            var delta = (lastY - mouse.y) / 100.0;
            lastY = mouse.y;
            root.value = Math.max(0.0, Math.min(1.0, root.value + delta));
            root.moved(root.value);
        }
    }
}