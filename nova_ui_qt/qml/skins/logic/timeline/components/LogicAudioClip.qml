import QtQuick
import NovaStudio 1.0

Item {
    id: root
    
    // Propiedades expuestas y vinculadas al modelo C++
    property real barWidth: 80.0
    property real startBeat: (typeof model.startBeat !== "undefined") ? model.startBeat : 0.0
    property real lengthBeats: (typeof model.lengthBeats !== "undefined") ? model.lengthBeats : 0.0
    property string clipName: model.regionName || "Audio Clip"
    property string clipColor: model.isLiveRecording ? "#FF3B30" : (model.regionColor || "#4A90E2")
    property real snapGridBeats: 1.0
    property bool isSelected: false
    property bool isLiveRecording: (typeof model.isLiveRecording !== "undefined") ? model.isLiveRecording : false

    // Propiedades de Ganancia y Fundidos (Fades)
    property real clipGainDb: (typeof model.clipGainDb !== "undefined") ? model.clipGainDb : 0.0
    property real fadeInBeats: (typeof model.fadeInBeats !== "undefined") ? model.fadeInBeats : 0.0
    property real fadeOutBeats: (typeof model.fadeOutBeats !== "undefined") ? model.fadeOutBeats : 0.0

    // Conversión de Beats a Píxeles para los Fades
    property real fadeInPixels: AudioEngine.beatToPixel(root.fadeInBeats, root.barWidth)
    property real fadeOutPixels: AudioEngine.beatToPixel(root.fadeOutBeats, root.barWidth)

    // Posición y ancho calculados con C++ Helpers
    x: AudioEngine.beatToPixel(startBeat, barWidth)
    width: root.isLiveRecording ? AudioEngine.beatToPixel(lengthBeats, barWidth) : Math.max(32, AudioEngine.beatToPixel(lengthBeats, barWidth))
    height: 66
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    Binding on x {
        when: !mainDragArea.drag.active && !leftHandleArea.pressed
        value: AudioEngine.beatToPixel(root.startBeat, root.barWidth)
    }
    
    Binding on width {
        when: !rightHandleArea.pressed && !leftHandleArea.pressed && !fadeInDragArea.pressed && !fadeOutDragArea.pressed
        value: root.isLiveRecording ? AudioEngine.beatToPixel(root.lengthBeats, root.barWidth) : Math.max(32, AudioEngine.beatToPixel(root.lengthBeats, root.barWidth))
    }

    // ── CONTENEDOR VISUAL DEL CLIP ──
    Rectangle {
        id: clipBg
        anchors.fill: parent
        radius: 4
        color: root.isLiveRecording ? "#D32F2F" : (mainDragArea.containsMouse || mainDragArea.drag.active ? Qt.lighter(clipColor, 1.15) : clipColor)
        border.color: root.isLiveRecording ? "#FF1744" : (root.isSelected ? "#FFCC00" : (mainDragArea.drag.active ? "#FFFFFF" : Qt.lighter(clipColor, 1.3)))
        border.width: root.isLiveRecording || root.isSelected ? 2 : (mainDragArea.drag.active ? 2 : 1)

        // Relieve interno 3D
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 3
            color: "transparent"
            border.color: root.isLiveRecording ? "#80FF5252" : (root.isSelected ? "#80FFCC00" : "#30FFFFFF")
            border.width: 1
        }

        // Barra superior de título (Optimizada para toque)
        Rectangle {
            id: titleBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 20
            color: root.isLiveRecording ? "#50000000" : (root.isSelected ? "#40FFCC00" : "#25000000")
            radius: 3
            z: 20

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

            // ⚡ BOTÓN "⚡ Auto-Gain" DE 1-TOQUE (Hitbox Táctil Ampliada)
            Rectangle {
                id: autoGainBtn
                anchors.right: deleteClipContainer.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: 64
                height: 16
                radius: 3
                color: autoGainMouseArea.containsMouse || autoGainMouseArea.pressed ? "#80FFCC00" : "#30FFFFFF"
                border.color: autoGainMouseArea.containsMouse || autoGainMouseArea.pressed ? "#FFCC00" : "transparent"
                border.width: 1
                visible: !root.isLiveRecording

                Text {
                    anchors.centerIn: parent
                    text: "⚡ Auto-Gain"
                    color: "#FFFFFF"
                    font.pixelSize: 8
                    font.bold: true
                }

                MouseArea {
                    id: autoGainMouseArea
                    anchors.fill: parent
                    anchors.margins: -4 // Hitbox táctil extra
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        AudioEngine.regions.normalizeClip(index)
                    }
                }
            }

            // 🗑️ BOTÓN DE BORRAR (Hitbox Táctil Ampliada 28x20px)
            Item {
                id: deleteClipContainer
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 20
                visible: !root.isLiveRecording

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: deleteClipMouse.containsMouse || deleteClipMouse.pressed ? "#FF3B30" : "#D0FFFFFF"
                    font.pixelSize: 11
                    font.bold: true
                }

                MouseArea {
                    id: deleteClipMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: AudioEngine.regions.removeRegion(index)
                }
            }
        }

        // 🎙️ WAVEFORM REAL VECTORIAL HD (Caché RAM en C++)
        NovaWaveformItem {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            regionIndex: model.index
            regionId: model.regionId || ""
            waveColor: "#FFFFFF"
            visible: true
        }

        // ── CAPAS DE GRADIENTES DE FUNDIDO (FADES OVERLAYS) ──
        
        // Sombra de Fade In (Rampa de entrada)
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            width: root.fadeInPixels
            visible: root.fadeInBeats > 0.001 && !root.isLiveRecording
            z: 2
            
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#70000000" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }

        // Sombra de Fade Out (Rampa de salida)
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            width: root.fadeOutPixels
            visible: root.fadeOutBeats > 0.001 && !root.isLiveRecording
            z: 2
            
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#70000000" }
            }
        }

        // ── CONTROLES TÁCTILES DINÁMICOS (CLIP GAIN & FADE HANDLES) ──

        // 🔊 LÍNEA HORIZONTAL DE CLIP GAIN (Hitbox de 24px para toque fácil)
        Item {
            id: gainLineItem
            anchors.left: parent.left
            anchors.right: parent.right
            y: 20 + 22 - (root.clipGainDb / 24.0) * 18 // Mapeo de [-24dB, 24dB]
            height: 24 // Zona táctil ampliada a 24px (Hitbox)
            z: 15
            visible: !root.isLiveRecording

            // Línea delgada visible en el centro
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: gainDragArea.containsMouse || gainDragArea.pressed ? 2 : 1
                color: gainDragArea.containsMouse || gainDragArea.pressed ? "#FFCC00" : "#80FFFFFF"
            }

            // Tooltip flotante de volumen en dB en tiempo real
            Rectangle {
                visible: gainDragArea.pressed
                anchors.horizontalCenter: parent.horizontalCenter
                y: -22
                width: gainDbText.implicitWidth + 12
                height: 18
                radius: 4
                color: "#181A20"
                border.color: "#FFCC00"
                border.width: 1

                Text {
                    id: gainDbText
                    anchors.centerIn: parent
                    color: "#FFFFFF"
                    font.pixelSize: 9
                    font.bold: true
                    text: (root.clipGainDb >= 0.0 ? "+" : "") + root.clipGainDb.toFixed(1) + " dB"
                }
            }

            MouseArea {
                id: gainDragArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeVerCursor
                property real startY: 0
                property real startGain: 0

                onPressed: (mouse) => {
                    root.isSelected = true
                    startY = mouse.y
                    startGain = root.clipGainDb
                }
                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var deltaY = startY - mouse.y
                        var dbChange = (deltaY / 18.0) * 24.0
                        var newGain = Math.max(-24.0, Math.min(24.0, startGain + dbChange))
                        AudioEngine.regions.setClipGainDb(index, newGain)
                    }
                }
            }
        }

        // ⚡ NODO DE CONTROL: FADE IN (Hitbox Táctil 36x36 px)
        Rectangle {
            id: fadeInNode
            x: Math.max(0, root.fadeInPixels - 7)
            y: 20
            width: 14; height: 14
            radius: 7
            color: fadeInDragArea.containsMouse || fadeInDragArea.pressed ? "#FFCC00" : "#FFFFFF"
            border.color: "#111111"
            border.width: 1
            z: 16
            visible: !root.isLiveRecording && (mainDragArea.containsMouse || root.isSelected || fadeInDragArea.containsMouse)

            MouseArea {
                id: fadeInDragArea
                anchors.fill: parent
                anchors.margins: -11 // Hitbox táctil de 36x36px para toque con dedo
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real pressParentX: 0
                property real startFadeBeats: 0

                onPressed: (mouse) => {
                    root.isSelected = true
                    var pt = mapToItem(root.parent, mouse.x, mouse.y)
                    pressParentX = pt.x
                    startFadeBeats = root.fadeInBeats
                }

                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var pt = mapToItem(root.parent, mouse.x, mouse.y)
                        var deltaX = pt.x - pressParentX
                        var deltaBeats = AudioEngine.pixelToBeat(deltaX, root.barWidth)
                        var maxFadeBeats = root.lengthBeats / 2.0
                        var newFade = Math.max(0.0, Math.min(maxFadeBeats, startFadeBeats + deltaBeats))
                        AudioEngine.regions.setFadeInBeats(index, newFade)
                    }
                }
            }
        }

        // ⚡ NODO DE CONTROL: FADE OUT (Hitbox Táctil 36x36 px)
        Rectangle {
            id: fadeOutNode
            x: Math.max(0, root.width - root.fadeOutPixels - 7)
            y: 20
            width: 14; height: 14
            radius: 7
            color: fadeOutDragArea.containsMouse || fadeOutDragArea.pressed ? "#FFCC00" : "#FFFFFF"
            border.color: "#111111"
            border.width: 1
            z: 16
            visible: !root.isLiveRecording && (mainDragArea.containsMouse || root.isSelected || fadeOutDragArea.containsMouse)

            MouseArea {
                id: fadeOutDragArea
                anchors.fill: parent
                anchors.margins: -11 // Hitbox táctil de 36x36px
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor
                property real pressParentX: 0
                property real startFadeBeats: 0

                onPressed: (mouse) => {
                    root.isSelected = true
                    var pt = mapToItem(root.parent, mouse.x, mouse.y)
                    pressParentX = pt.x
                    startFadeBeats = root.fadeOutBeats
                }

                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var pt = mapToItem(root.parent, mouse.x, mouse.y)
                        var deltaX = pressParentX - pt.x
                        var deltaBeats = AudioEngine.pixelToBeat(deltaX, root.barWidth)
                        var maxFadeBeats = root.lengthBeats / 2.0
                        var newFade = Math.max(0.0, Math.min(maxFadeBeats, startFadeBeats + deltaBeats))
                        AudioEngine.regions.setFadeOutBeats(index, newFade)
                    }
                }
            }
        }

        // ── TIRADORES TÁCTILES DE CORTE (RESIZE HANDLES) ──
        
        // Tirador de recorte Izquierdo (Ancho 16px para dedos)
        Rectangle {
            id: leftHandle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            width: 16
            color: leftHandleArea.containsMouse || leftHandleArea.pressed ? "#40FFFFFF" : "transparent"
            z: 10
            visible: !root.isLiveRecording

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
                        var maxRight = initialX + initialWidth - 32
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

        // Tirador de recorte Derecho (Ancho 16px para dedos)
        Rectangle {
            id: rightHandle
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            width: 16
            color: rightHandleArea.containsMouse || rightHandleArea.pressed ? "#40FFFFFF" : "transparent"
            z: 10
            visible: !root.isLiveRecording

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
                        root.width = Math.max(32, initialWidth + delta)
                    }
                }

                onReleased: {
                    var newLengthBeats = Math.max(0.5, AudioEngine.pixelToBeat(root.width, root.barWidth))
                    AudioEngine.regions.resizeRegion(index, root.startBeat, newLengthBeats)
                }
            }
        }

        // ── ÁREA CENTRAL DE ARRASTRE (DRAG & DROP) ──
        MouseArea {
            id: mainDragArea
            anchors.left: leftHandle.right
            anchors.right: rightHandle.left
            anchors.top: parent.top
            anchors.topMargin: 20
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

    // Tooltip Flotante Táctil de Posición y Fades
    Rectangle {
        id: positionTooltip
        visible: mainDragArea.drag.active || leftHandleArea.pressed || rightHandleArea.pressed || fadeInDragArea.pressed || fadeOutDragArea.pressed
        anchors.horizontalCenter: parent.horizontalCenter
        y: -28
        width: tooltipText.implicitWidth + 16
        height: 22
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
                if (fadeInDragArea.pressed) {
                    return "Fade In: " + root.fadeInBeats.toFixed(2) + " Beats";
                }
                if (fadeOutDragArea.pressed) {
                    return "Fade Out: " + root.fadeOutBeats.toFixed(2) + " Beats";
                }
                var currentBeats = AudioEngine.pixelToBeat(root.x, root.barWidth)
                var bar = 1 + Math.floor(currentBeats / 4.0)
                var beat = 1 + Math.floor(currentBeats % 4.0)
                var durationBeats = AudioEngine.pixelToBeat(root.width, root.barWidth).toFixed(1)
                return "Compás " + bar + " : Beat " + beat + "  (" + durationBeats + " b)";
            }
        }
    }
}