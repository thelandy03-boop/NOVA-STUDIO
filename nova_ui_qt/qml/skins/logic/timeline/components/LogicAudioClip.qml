import QtQuick
import NovaStudio 1.0 // Importamos nuestro tipo nativo C++

Item {
    id: root
    
    property real barWidth: 80.0
    property real startBeat: model.startBeat || 0
    property real lengthBeats: model.lengthBeats || 4.0
    property string clipName: model.regionName || "Audio Clip"
    property string clipColor: model.regionColor || "#4A90E2"
    property real snapGridBeats: 1.0

    x: (startBeat / 4.0) * barWidth
    width: Math.max(24, (lengthBeats / 4.0) * barWidth)
    height: 66
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    Binding on x {
        when: !mainDragArea.drag.active && !leftHandleArea.pressed
        value: (root.startBeat / 4.0) * root.barWidth
    }
    Binding on width {
        when: !rightHandleArea.pressed && !leftHandleArea.pressed
        value: Math.max(24, (root.lengthBeats / 4.0) * root.barWidth)
    }

    Rectangle {
        id: clipBg
        anchors.fill: parent
        radius: 4
        color: mainDragArea.containsMouse || mainDragArea.drag.active ? Qt.lighter(clipColor, 1.15) : clipColor
        border.color: mainDragArea.drag.active ? "#FFFFFF" : Qt.lighter(clipColor, 1.3)
        border.width: mainDragArea.drag.active ? 2 : 1

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 3
            color: "transparent"
            border.color: "#30FFFFFF"
            border.width: 1
        }

        // Cabecera de título
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 18
            color: "#25000000"
            radius: 3
            z: 5

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.clipName
                color: "#FFFFFF"
                font.pixelSize: 10
                font.bold: true
                elide: Text.ElideRight
            }
        }

        // 🎨 FORMA DE ONDA REAL DIBUJADA CON C++ Y QPAINTER
        NovaWaveformItem {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 18
            anchors.bottom: parent.bottom
            regionIndex: model.index
            waveColor: "#E0FFFFFF"
        }

        // ── TIRADOR IZQUIERDO ──
        Rectangle {
            id: leftHandle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 12
            color: leftHandleArea.containsMouse || leftHandleArea.pressed ? "#A0FFFFFF" : "#20FFFFFF"
            radius: 2
            z: 10

            Rectangle { width: 2; height: 16; color: "#FFFFFF"; anchors.centerIn: parent }

            MouseArea {
                id: leftHandleArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real initialX: 0
                property real initialWidth: 0
                property real pressParentX: 0

                onPressed: (mouse) => {
                    initialX = root.x
                    initialWidth = root.width
                    var pt = mapToItem(root.parent, mouse.x, mouse.y)
                    pressParentX = pt.x
                }

                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var pt = mapToItem(root.parent, mouse.x, mouse.y)
                        var delta = pt.x - pressParentX
                        var maxRight = initialX + initialWidth - 24
                        var newX = Math.max(0, Math.min(maxRight, initialX + delta))
                        var newWidth = initialWidth - (newX - initialX)

                        root.x = newX
                        root.width = newWidth
                    }
                }

                onReleased: {
                    var pixelsPerBeat = root.barWidth / 4.0
                    var rawBeat = root.x / pixelsPerBeat
                    var snappedBeat = Math.max(0, Math.round(rawBeat / root.snapGridBeats) * root.snapGridBeats)
                    
                    var deltaBeat = snappedBeat - (root.x / pixelsPerBeat)
                    var newLengthBeats = Math.max(0.5, (root.width / pixelsPerBeat) - deltaBeat)

                    root.x = (snappedBeat / 4.0) * root.barWidth
                    root.width = (newLengthBeats / 4.0) * root.barWidth

                    AudioEngine.regions.resizeRegion(index, snappedBeat, newLengthBeats)
                }
            }
        }

        // ── TIRADOR DERECHO ──
        Rectangle {
            id: rightHandle
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 12
            color: rightHandleArea.containsMouse || rightHandleArea.pressed ? "#A0FFFFFF" : "#20FFFFFF"
            radius: 2
            z: 10

            Rectangle { width: 2; height: 16; color: "#FFFFFF"; anchors.centerIn: parent }

            MouseArea {
                id: rightHandleArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real initialWidth: 0
                property real pressParentX: 0

                onPressed: (mouse) => {
                    initialWidth = root.width
                    var pt = mapToItem(root.parent, mouse.x, mouse.y)
                    pressParentX = pt.x
                }

                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var pt = mapToItem(root.parent, mouse.x, mouse.y)
                        var delta = pt.x - pressParentX
                        root.width = Math.max(24, initialWidth + delta)
                    }
                }

                onReleased: {
                    var pixelsPerBeat = root.barWidth / 4.0
                    var newLengthBeats = Math.max(0.5, root.width / pixelsPerBeat)
                    AudioEngine.regions.resizeRegion(index, root.startBeat, newLengthBeats)
                }
            }
        }

        // ── ÁREA CENTRAL DRAG ──
        MouseArea {
            id: mainDragArea
            anchors.left: leftHandle.right
            anchors.right: rightHandle.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            hoverEnabled: true
            cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            drag.target: root
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: root.parent ? root.parent.width - root.width : 4800

            onReleased: {
                var pixelsPerBeat = root.barWidth / 4.0
                var rawBeat = root.x / pixelsPerBeat
                var snappedBeat = Math.max(0, Math.round(rawBeat / root.snapGridBeats) * root.snapGridBeats)

                root.x = (snappedBeat / 4.0) * root.barWidth
                AudioEngine.regions.moveRegion(index, snappedBeat)
            }
        }
    }

    // Tooltip
    Rectangle {
        id: positionTooltip
        visible: mainDragArea.drag.active || leftHandleArea.pressed || rightHandleArea.pressed
        anchors.horizontalCenter: parent.horizontalCenter
        y: -26
        width: tooltipText.implicitWidth + 16
        height: 20
        radius: 4
        color: "#181A20"
        border.color: "#4A4A4A"
        border.width: 1
        z: 999

        Text {
            id: tooltipText
            anchors.centerIn: parent
            color: "#FFFFFF"
            font.pixelSize: 11
            font.bold: true
            text: {
                var pixelsPerBeat = root.barWidth / 4.0
                var currentBeats = (root.x / pixelsPerBeat)
                var bar = 1 + Math.floor(currentBeats / 4.0)
                var beat = 1 + Math.floor(currentBeats % 4.0)
                var durationBeats = (root.width / pixelsPerBeat).toFixed(1)
                return "Compás " + bar + " : Beat " + beat + "  (" + durationBeats + " b)";
            }
        }
    }
}