import QtQuick
import NovaStudio 1.0

Item {
    id: root
    
    // Propiedades expuestas
    property real barWidth: 80.0
    property real startBeat: (typeof model.startBeat !== "undefined") ? model.startBeat : 0.0
    property real lengthBeats: (typeof model.lengthBeats !== "undefined") ? model.lengthBeats : 0.0
    property string clipName: model.regionName || "Audio Clip"
    property string clipColor: model.isLiveRecording ? "#FF3B30" : (model.regionColor || "#4A90E2")
    property real snapGridBeats: 1.0
    property bool isSelected: false
    property bool isLiveRecording: (typeof model.isLiveRecording !== "undefined") ? model.isLiveRecording : false

    // Posición y ancho calculados con C++ Helpers
    x: AudioEngine.beatToPixel(startBeat, barWidth)
    width: root.isLiveRecording ? AudioEngine.beatToPixel(lengthBeats, barWidth) : Math.max(24, AudioEngine.beatToPixel(lengthBeats, barWidth))
    height: 66
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    Binding on x {
        when: !mainDragArea.drag.active && !leftHandleArea.pressed
        value: AudioEngine.beatToPixel(root.startBeat, root.barWidth)
    }
    
    Binding on width {
        when: !rightHandleArea.pressed && !leftHandleArea.pressed
        value: root.isLiveRecording ? AudioEngine.beatToPixel(root.lengthBeats, root.barWidth) : Math.max(24, AudioEngine.beatToPixel(root.lengthBeats, root.barWidth))
    }

    // ── CONTENEDOR VISUAL DEL CLIP ──
    Rectangle {
        id: clipBg
        anchors.fill: parent
        radius: 4
        color: root.isLiveRecording ? "#D32F2F" : (mainDragArea.containsMouse || mainDragArea.drag.active ? Qt.lighter(clipColor, 1.15) : clipColor)
        border.color: root.isLiveRecording ? "#FF1744" : (root.isSelected ? "#FFCC00" : (mainDragArea.drag.active ? "#FFFFFF" : Qt.lighter(clipColor, 1.3)))
        border.width: root.isLiveRecording || root.isSelected ? 2 : (mainDragArea.drag.active ? 2 : 1)

        // Relieve interno
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 3
            color: "transparent"
            border.color: root.isLiveRecording ? "#80FF5252" : (root.isSelected ? "#80FFCC00" : "#30FFFFFF")
            border.width: 1
        }

        // Barra superior de título
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 18
            color: root.isLiveRecording ? "#50000000" : (root.isSelected ? "#40FFCC00" : "#25000000")
            radius: 3
            z: 5

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: "#FFFFFF"
                    visible: root.isLiveRecording
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        running: root.isLiveRecording
                        loops: Animation.Infinite
                        PropertyAnimation { to: 0.2; duration: 400 }
                        PropertyAnimation { to: 1.0; duration: 400 }
                    }
                }

                Text {
                    text: root.clipName
                    color: "#FFFFFF"
                    font.pixelSize: 10
                    font.bold: true
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: "✕"
                color: deleteClipArea.containsMouse ? "#FF3B30" : "#A0FFFFFF"
                font.pixelSize: 10
                font.bold: true
                visible: !root.isLiveRecording

                MouseArea {
                    id: deleteClipArea
                    anchors.fill: parent
                    anchors.margins: -4
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: AudioEngine.regions.removeRegion(index)
                }
            }
        }

        NovaWaveformItem {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 18
            anchors.bottom: parent.bottom
            regionIndex: model.index
            waveColor: "#E0FFFFFF"
            visible: !root.isLiveRecording
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            visible: root.isLiveRecording
            clip: true

            Row {
                anchors.centerIn: parent
                spacing: 2
                Repeater {
                    model: Math.max(0, Math.min(80, Math.floor(root.width / 3)))
                    Rectangle {
                        width: 2
                        height: (index % 4 === 0 ? 32 : (index % 2 === 0 ? 20 : 12))
                        color: "#FFFFFF"
                        opacity: 0.85
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
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
            visible: !root.isLiveRecording

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
                    root.isSelected = true
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
                    var rawBeat = AudioEngine.pixelToBeat(root.x, root.barWidth)
                    var snappedBeat = Math.max(0, Math.round(rawBeat / root.snapGridBeats) * root.snapGridBeats)
                    
                    var currentLengthBeats = AudioEngine.pixelToBeat(root.width, root.barWidth)
                    var deltaBeat = snappedBeat - rawBeat
                    var newLengthBeats = Math.max(0.5, currentLengthBeats - deltaBeat)

                    root.x = AudioEngine.beatToPixel(snappedBeat, root.barWidth)
                    root.width = AudioEngine.beatToPixel(newLengthBeats, root.barWidth)

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
            visible: !root.isLiveRecording

            Rectangle { width: 2; height: 16; color: "#FFFFFF"; anchors.centerIn: parent }

            MouseArea {
                id: rightHandleArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real initialWidth: 0
                property real pressParentX: 0

                onPressed: (mouse) => {
                    root.isSelected = true
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
                    var newLengthBeats = Math.max(0.5, AudioEngine.pixelToBeat(root.width, root.barWidth))
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
            enabled: !root.isLiveRecording
            cursorShape: drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            drag.target: root
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: root.parent ? root.parent.width - root.width : 4800

            onPressed: {
                root.isSelected = true
            }

            onReleased: {
                var rawBeat = AudioEngine.pixelToBeat(root.x, root.barWidth)
                var snappedBeat = Math.max(0, Math.round(rawBeat / root.snapGridBeats) * root.snapGridBeats)

                root.x = AudioEngine.beatToPixel(snappedBeat, root.barWidth)
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
        border.color: "#FFCC00"
        border.width: 1
        z: 999

        Text {
            id: tooltipText
            anchors.centerIn: parent
            color: "#FFFFFF"
            font.pixelSize: 11
            font.bold: true
            text: {
                var currentBeats = AudioEngine.pixelToBeat(root.x, root.barWidth)
                var bar = 1 + Math.floor(currentBeats / 4.0)
                var beat = 1 + Math.floor(currentBeats % 4.0)
                var durationBeats = AudioEngine.pixelToBeat(root.width, root.barWidth).toFixed(1)
                return "Compás " + bar + " : Beat " + beat + "  (" + durationBeats + " b)";
            }
        }
    }
}