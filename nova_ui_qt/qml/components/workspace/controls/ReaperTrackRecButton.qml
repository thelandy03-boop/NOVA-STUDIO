import QtQuick

ReaperBaseButton {
    id: root

    implicitWidth: 20
    implicitHeight: 20

    // Alias para que no rompa tu código que usa "armed"
    property alias armed: root.active

    // 🔴 Círculo exterior (20x20)
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.armed ? "#C9264F" : "#633942"
        border.color: "#B65D62"
        border.width: 1.5
        antialiasing: true

        // ⭕ Anillo concéntrico PAR (12x12 -> 4px exactos de margen en los 4 lados)
        Rectangle {
            width: 12
            height: 12
            radius: width / 2
            anchors.centerIn: parent
            color: "transparent"
            border.color: root.armed ? "#FFFFFF" : "#AC8B90"
            border.width: 2
            antialiasing: true
        }
    }
}