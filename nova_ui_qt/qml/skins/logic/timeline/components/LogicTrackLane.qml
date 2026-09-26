import QtQuick

Rectangle {
    id: root
    width: parent ? parent.width : 4800
    height: 74
    color: (index % 2 === 0) ? "#1E1F24" : "#191A1E"

    property int trackIndex: index

    // Franja lateral de color
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: model.trackColor || "#3498db"
    }

    // Separador inferior
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#111216"
    }

    // Nombre de marca de agua
    Text {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: model.trackName
        color: "#2C2F38"
        font.pixelSize: 18
        font.bold: true
    }

    // 🎵 RENDERIZADO DINÁMICO DE CLIPS DE AUDIO CORRESPONDIENTES A ESTA PISTA
    Repeater {
        model: AudioEngine.regions
        delegate: LogicAudioClip {
            // Se dibuja solo si el clip pertenece a este carril/pista
            visible: model.trackIndex === root.trackIndex
            barWidth: 80
        }
    }
}