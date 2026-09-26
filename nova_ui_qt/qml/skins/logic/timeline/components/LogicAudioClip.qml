import QtQuick

Rectangle {
    id: root
    
    // Propiedades expuestas
    property real barWidth: 80.0
    property real startBeat: model.startBeat || 0
    property real lengthBeats: model.lengthBeats || 4.0
    property string clipName: model.regionName || "Audio Clip"
    property string clipColor: model.regionColor || "#4A90E2"

    // Resolución del imán/grid: 1.0 = 1 Beat (Negra), 4.0 = 1 Compás
    property real snapGridBeats: 1.0

    // Posicionamiento en píxeles basado en beats (4 beats por compás de 80px -> 20px por beat)
    x: (startBeat / 4.0) * barWidth
    width: Math.max(24, (lengthBeats / 4.0) * barWidth)
    height: 66
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    radius: 4
    color: clipColor
    border.color: Qt.lighter(clipColor, 1.3)
    border.width: 1

    // Relieve visual de clip
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
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.clipName
            color: "#FFFFFF"
            font.pixelSize: 10
            font.bold: true
            elide: Text.ElideRight
        }
    }

    // Silueta de ondas de audio
    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4

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

    // ── TOOLTIP EMERGENTE DE POSICIÓN MUSICAL (Bar : Beat) ──
    Rectangle {
        id: positionTooltip
        visible: mainDragArea.pressed || rightHandleArea.pressed
        anchors.horizontalCenter: parent.horizontalCenter
        y: -24
        width: tooltipText.implicitWidth + 12
        height: 18
        radius: 3
        color: "#1E2026"
        border.color: "#4A90E2"
        border.width: 1

        Text {
            id: tooltipText
            anchors.centerIn: parent
            color: "#FFFFFF"
            font.pixelSize: 10
            font.bold: true
            text: {
                var currentBeats = (root.x / (root.barWidth / 4.0));
                var bar = 1 + Math.floor(currentBeats / 4.0);
                var beat = 1 + Math.floor(currentBeats % 4.0);
                return "Compás " + bar + " : Beat " + beat;
            }
        }
    }

    // ── ÁREA PRINCIPAL PARA ARRASTRAR EL CLIP CON IMÁN ──
    MouseArea {
        id: mainDragArea
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        cursorShape: Qt.SizeAllCursor

        property real dragStartX: 0

        onPressed: (mouse) => {
            dragStartX = mouse.x
        }

        onPositionChanged: (mouse) => {
            if (pressed) {
                var deltaX = mouse.x - dragStartX
                var rawX = Math.max(0, root.x + deltaX)
                
                // Mapear X a beats y aplicar IMÁN (Snap)
                var pixelsPerBeat = root.barWidth / 4.0
                var rawBeat = rawX / pixelsPerBeat
                var snappedBeat = Math.max(0, Math.round(rawBeat / root.snapGridBeats) * root.snapGridBeats)
                
                // Posicionar visualmente
                root.x = (snappedBeat / 4.0) * root.barWidth
            }
        }

        onReleased: {
            var pixelsPerBeat = root.barWidth / 4.0
            var finalBeat = Math.max(0, Math.round((root.x / pixelsPerBeat) / root.snapGridBeats) * root.snapGridBeats)
            
            // Actualizar en Ardour Core
            AudioEngine.regions.moveRegion(index, finalBeat)
        }
    }

    // ── TIRADOR DERECHO PARA RECORTE (Trimming Right) ──
    Rectangle {
        id: rightHandle
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 8
        color: rightHandleArea.containsMouse || rightHandleArea.pressed ? "#64B5F6" : "transparent"
        radius: 2

        MouseArea {
            id: rightHandleArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor

            property real handlePressX: 0

            onPressed: (mouse) => handlePressX = mouse.x

            onPositionChanged: (mouse) => {
                if (pressed) {
                    var deltaX = mouse.x - handlePressX
                    var newWidth = Math.max(20, root.width + deltaX)
                    root.width = newWidth
                }
            }

            onReleased: {
                var pixelsPerBeat = root.barWidth / 4.0
                var newLengthBeats = Math.max(0.5, root.width / pixelsPerBeat)
                
                // Actualizar recorte en Ardour Core
                AudioEngine.regions.resizeRegion(index, root.startBeat, newLengthBeats)
            }
        }
    }
}