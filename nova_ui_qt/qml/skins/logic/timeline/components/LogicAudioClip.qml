import QtQuick

Rectangle {
    id: root
    
    // Propiedades expuestas
    property real barWidth: 80.0
    property real startBeat: model.startBeat || 0
    property real lengthBeats: model.lengthBeats || 4.0
    property string clipName: model.regionName || "Audio Clip"
    property string clipColor: model.regionColor || "#4A90E2"

    // Posicionamiento dinámico en el grid (4 beats por compás)
    x: (startBeat / 4.0) * barWidth
    width: Math.max(20, (lengthBeats / 4.0) * barWidth)
    height: 66 // Cabe holgadamente dentro del carril de 74px de alto
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    radius: 4
    color: clipColor
    border.color: Qt.lighter(clipColor, 1.3)
    border.width: 1

    // Relieve y brillo interno de audio clip
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: 3
        color: "transparent"
        border.color: "#30FFFFFF"
        border.width: 1
    }

    // Cabecera del clip con el nombre
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 18
        color: "#20000000"
        radius: 3

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: root.clipName
            color: "#FFFFFF"
            font.pixelSize: 10
            font.bold: true
            elide: Text.ElideRight
        }
    }

    // ÁREA DE FORMA DE ONDA / SILUETA
    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4

        // Representación visual de ondas de audio (Línea decorativa simétrica)
        Row {
            anchors.centerIn: parent
            spacing: 2
            Repeater {
                model: Math.min(30, Math.floor(root.width / 4))
                Rectangle {
                    width: 2
                    height: (index % 3 === 0 ? 28 : (index % 2 === 0 ? 18 : 10))
                    color: "#A0FFFFFF"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // MouseArea para arrastrar el clip por los compases
    MouseArea {
        id: dragArea
        anchors.fill: parent
        cursorShape: Qt.SizeHorCursor
        property real pressX: 0

        onPressed: (mouse) => pressX = mouse.x
        onPositionChanged: (mouse) => {
            if (pressed) {
                var deltaX = mouse.x - pressX
                var newX = Math.max(0, root.x + deltaX)
                root.x = newX
            }
        }
    }
}