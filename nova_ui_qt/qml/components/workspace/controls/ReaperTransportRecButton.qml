import QtQuick

ReaperBaseButton {
    id: root

    implicitWidth: 28
    implicitHeight: 28

    // Alias para que no rompa tu código que usa "recording"
    property alias recording: root.active

    // 🎨 Solo definimos el dibujo
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#3A3A3A"
        border.color: root.recording ? "#FF2222" : "#151515"
        border.width: 1
        antialiasing: true

        Rectangle {
            width: parent.width * 0.5
            height: parent.height * 0.5
            radius: width / 2
            anchors.centerIn: parent
            color: root.recording ? "#FF2222" : "#D32F2F"
            antialiasing: true

            Rectangle {
                width: parent.width * 0.35
                height: parent.height * 0.35
                radius: width / 2
                anchors.centerIn: parent
                color: "#3A3A3A"
                antialiasing: true
            }
        }
    }
}