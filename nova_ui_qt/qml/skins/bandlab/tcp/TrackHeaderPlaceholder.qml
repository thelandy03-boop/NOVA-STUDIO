import QtQuick

Rectangle {
    id: root
    implicitWidth: 280
    implicitHeight: 82
    width: parent ? parent.width : 280
    color: "#0E0F13"

    // Placeholder Skeleton Visual (Opacity .5)
    Item {
        anchors.fill: parent
        opacity: 0.5

        // Icono circular pista (x=6 y=5)
        Rectangle { x: 8; y: 8; width: 20; height: 20; radius: 10; color: "#3A3D4C" }

        // Nombre de pista bar (x=36 y=10)
        Rectangle { x: 36; y: 13; width: 140; height: 10; radius: 5; color: "#3A3D4C" }

        // Botones M/S (x=197 y=5)
        Rectangle { x: 190; y: 8; width: 44; height: 20; radius: 4; color: "#3A3D4C" }

        // Menu redondo (x=252 y=5)
        Rectangle { x: 246; y: 8; width: 20; height: 20; radius: 10; color: "#3A3D4C" }

        // Pill +Fx (x=36 y=32)
        Rectangle { x: 36; y: 34; width: 34; height: 16; radius: 8; color: "#3A3D4C" }

        // Slider Fader bar (x=36 y=60)
        Rectangle { x: 36; y: 60; width: 180; height: 5; radius: 2.5; color: "#3A3D4C" }

        // Pan Knob (x=240 y=50)
        Rectangle { x: 236; y: 50; width: 24; height: 24; radius: 12; color: "#3A3D4C" }
    }
}