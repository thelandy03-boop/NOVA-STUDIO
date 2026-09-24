import QtQuick

Rectangle {
    id: root
    width: 22
    height: 32
    radius: 3
    border.color: "#111111"
    border.width: 1
    antialiasing: true

    // Gradiente metálico de 9 paradas (Scene Graph GPU)
    gradient: Gradient {
        GradientStop { position: 0.00; color: "#E8E8E8" }
        GradientStop { position: 0.15; color: "#FFFFFF" }
        GradientStop { position: 0.30; color: "#555555" }
        GradientStop { position: 0.48; color: "#333333" }
        GradientStop { position: 0.50; color: "#000000" } // Muesca / Ranura central
        GradientStop { position: 0.52; color: "#888888" }
        GradientStop { position: 0.70; color: "#444444" }
        GradientStop { position: 0.85; color: "#E8E8E8" }
        GradientStop { position: 1.00; color: "#666666" }
    }

    // ☀️ Bisel brillante izquierdo (Insetizado 2px para no sobresalir de las curvas)
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 1
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        width: 1
        color: "#FFFFFF"
        opacity: 0.35
        antialiasing: true
    }

    // 🌑 Bisel oscuro derecho (Insetizado 2px para no sobresalir)
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 1
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        width: 1
        color: "#000000"
        opacity: 0.4
        antialiasing: true
    }
}