import QtQuick

Rectangle {
    id: root
    implicitWidth: 4800
    implicitHeight: 28
    color: "#25272E"

    property real barWidth: 80.0
    property real contentX: 0

    // Borde inferior
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#14151A"
    }

    // ── BARRA VISUAL DE BUCLE (LOOP BAR) ──
    Rectangle {
        id: loopBar
        y: 2
        height: 8
        color: AudioEngine.loopEnabled ? "#E6B800" : "#555A66"
        opacity: 0.85
        radius: 2
        z: 2

        property real startBeat: AudioEngine.loopStartBeat
        property real endBeat: AudioEngine.loopEndBeat

        x: (startBeat / 4.0) * root.barWidth - root.contentX
        width: Math.max(16, ((endBeat - startBeat) / 4.0) * root.barWidth)

        // Tirador izquierdo del bucle
        Rectangle {
            width: 6; height: parent.height
            anchors.left: parent.left
            color: "#FFFFFF"
            radius: 1

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var pt = mapToItem(root, mouse.x, mouse.y)
                        var rawX = pt.x + root.contentX
                        var newStartBeat = Math.max(0, Math.floor((rawX / (root.barWidth / 4.0))))
                        if (newStartBeat < AudioEngine.loopEndBeat) {
                            AudioEngine.setLoopRange(newStartBeat, AudioEngine.loopEndBeat)
                        }
                    }
                }
            }
        }

        // Tirador derecho del bucle
        Rectangle {
            width: 6; height: parent.height
            anchors.right: parent.right
            color: "#FFFFFF"
            radius: 1

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var pt = mapToItem(root, mouse.x, mouse.y)
                        var rawX = pt.x + root.contentX
                        var newEndBeat = Math.max(AudioEngine.loopStartBeat + 1, Math.ceil((rawX / (root.barWidth / 4.0))))
                        AudioEngine.setLoopRange(AudioEngine.loopStartBeat, newEndBeat)
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            cursorShape: Qt.PointingHandCursor
            onClicked: AudioEngine.toggleLoop()
        }
    }

    // ── MARCAS DE COMPASES Y NÚMEROS ──
    Row {
        x: -root.contentX
        anchors.top: loopBar.bottom
        anchors.topMargin: 2
        anchors.bottom: parent.bottom

        Repeater {
            model: 60 // 60 Compases

            Item {
                width: root.barWidth
                height: parent.height

                // Número de compás
                Text {
                    x: 4; y: 0
                    text: (index + 1).toString()
                    color: "#A0A5B5"
                    font.pixelSize: 10
                    font.bold: true
                }

                // Línea vertical principal
                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    height: 8
                    width: 1
                    color: "#606575"
                }

                // Sub-divisiones (Beats 2, 3, 4)
                Row {
                    anchors.fill: parent

                    Repeater {
                        model: 4
                        Item {
                            width: root.barWidth / 4
                            height: parent.height

                            Rectangle {
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                height: index === 0 ? 0 : 4
                                width: 1
                                color: "#404555"
                            }
                        }
                    }
                }
            }
        }
    }

    // ── INTERACCIÓN CLIC Y ARRASTRE DE AGUJA (SCRUBBING) ──
    MouseArea {
        id: rulerScrubArea
        anchors.fill: parent
        anchors.topMargin: 10 // Deja el bucle libre arriba
        cursorShape: Qt.PointingHandCursor

        function updatePlayheadPosition(mouse) {
            // 🛑 TOPE ESTRICTO: No permite valores a la izquierda del compás 1 (x < 0)
            var rawX = Math.max(0, mouse.x + root.contentX)
            var pixelsPerBeat = root.barWidth / 4.0
            var targetBeat = rawX / pixelsPerBeat
            AudioEngine.locateBeat(targetBeat)
        }

        onPressed: (mouse) => updatePlayheadPosition(mouse)
        onPositionChanged: (mouse) => {
            if (pressed) updatePlayheadPosition(mouse)
        }
    }
}