import QtQuick
import QtQuick.Layouts
import "../../theme"
import "controls"

Rectangle {
    id: root
    height: 44
    color: "#282828"

    property bool isRecording: false
    property bool isPlaying: false
    property bool isPaused: false

    // Bisel superior de división
    Rectangle {
        width: parent.width
        height: 1
        color: "#181818"
        anchors.top: parent.top
    }

    // ──────────────────────────────────────────────
    // ICONOS VECTORIALES DE TRANSPORTE
    // ──────────────────────────────────────────────
    component GeoIcon: Item {
        id: ico
        property string name: "play"
        property color ink: "#D0D0D0"

        implicitWidth: 14
        implicitHeight: 14
        width: 14
        height: 14

        Canvas {
            id: cvs
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = ico.ink
                ctx.strokeStyle = ico.ink
                ctx.lineWidth = 1.5
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                if (ico.name === "toStart") {
                    ctx.fillRect(2, 3, 2, 8)
                    ctx.beginPath()
                    ctx.moveTo(12, 3); ctx.lineTo(4, 7); ctx.lineTo(12, 11)
                    ctx.closePath(); ctx.fill()
                } else if (ico.name === "toEnd") {
                    ctx.beginPath()
                    ctx.moveTo(2, 3); ctx.lineTo(10, 7); ctx.lineTo(2, 11)
                    ctx.closePath(); ctx.fill()
                    ctx.fillRect(10, 3, 2, 8)
                } else if (ico.name === "play") {
                    ctx.beginPath()
                    ctx.moveTo(3.5, 2.5); ctx.lineTo(11.5, 7); ctx.lineTo(3.5, 11.5)
                    ctx.closePath(); ctx.fill()
                } else if (ico.name === "pause") {
                    ctx.fillRect(3.5, 3, 2.5, 8)
                    ctx.fillRect(8.0, 3, 2.5, 8)
                } else if (ico.name === "stop") {
                    ctx.fillRect(3, 3, 8, 8)
                } else if (ico.name === "loop") {
                    ctx.beginPath()
                    ctx.arc(7, 7, 4.5, Math.PI * 0.15, Math.PI * 1.6)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(11.5, 6)
                    ctx.lineTo(14, 9)
                    ctx.lineTo(9.5, 9.5)
                    ctx.closePath(); ctx.fill()
                }
            }
            Component.onCompleted: requestPaint()
        }
    }

    // BOTÓN DE TRANSPORTE PLANO REAPER
    component Tbtn: Rectangle {
        id: b
        property string icon: "play"
        property bool active: false
        property color activeColor: "#3DDC84"
        signal clicked()

        width: 28
        height: 28
        radius: 14
        color: active ? activeColor : (pressed ? "#1A1A1A" : "#2F2F31")
        border.color: active ? "#80FFFFFF" : "#141414"
        border.width: 1
        antialiasing: true

        GeoIcon {
            anchors.centerIn: parent
            name: b.icon
            ink: b.active ? "#000000" : "#D8D8D8"
        }

        property bool pressed: false
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: b.pressed = true
            onReleased: b.pressed = false
            onClicked: b.clicked()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 12

        // ──────────────────────────────────────────────
        // 1. CÁPSULAS IZQUIERDAS DE TRANSPORTE
        // ──────────────────────────────────────────────
        Row {
            spacing: 6
            Layout.alignment: Qt.AlignVCenter

            // CÁPSULA 1: NAVEGACIÓN
            Rectangle {
                height: 34
                width: group1Row.implicitWidth + 10
                radius: 17
                color: "#181818"
                border.color: "#101010"
                border.width: 1
                antialiasing: true

                Row {
                    id: group1Row
                    anchors.centerIn: parent
                    spacing: 3

                    Tbtn { icon: "toStart"; anchors.verticalCenter: parent.verticalCenter }
                    Tbtn { icon: "toEnd"; anchors.verticalCenter: parent.verticalCenter }
                }
            }

            // CÁPSULA 2: REC / PLAY / LOOP
            Rectangle {
                height: 34
                width: group2Row.implicitWidth + 10
                radius: 17
                color: "#181818"
                border.color: "#101010"
                border.width: 1
                antialiasing: true

                Row {
                    id: group2Row
                    anchors.centerIn: parent
                    spacing: 3

                    ReaperTrackRecButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        width: 28
                        height: 28
                        anchors.verticalCenter: parent.verticalCenter
                        armed: root.isRecording
                        onToggled: root.isRecording = armed
                    }

                    Tbtn {
                        icon: "play"
                        active: root.isPlaying
                        activeColor: "#3DDC84"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            root.isPlaying = !root.isPlaying
                            root.isPaused = false
                        }
                    }

                    Tbtn { icon: "loop"; anchors.verticalCenter: parent.verticalCenter }
                }
            }

            // CÁPSULA 3: STOP / PAUSE
            Rectangle {
                height: 34
                width: group3Row.implicitWidth + 10
                radius: 17
                color: "#181818"
                border.color: "#101010"
                border.width: 1
                antialiasing: true

                Row {
                    id: group3Row
                    anchors.centerIn: parent
                    spacing: 3

                    Tbtn {
                        icon: "stop"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            root.isPlaying = false
                            root.isPaused = false
                        }
                    }

                    Tbtn {
                        icon: "pause"
                        active: root.isPaused
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            root.isPaused = !root.isPaused
                            if (root.isPaused) root.isPlaying = false
                        }
                    }
                }
            }
        }

        // ──────────────────────────────────────────────
        // 2. RELOJ DE TIEMPO
        // ──────────────────────────────────────────────
        Row {
            spacing: 12
            Layout.alignment: Qt.AlignVCenter

            Text {
                text: "1.1.00  /  0:00.000"
                color: "#FFFFFF"
                font.family: "Consolas"
                font.pixelSize: 19
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.isRecording ? "[Recording]" : (root.isPlaying ? "[Playing]" : (root.isPaused ? "[Paused]" : "[Stopped]"))
                color: root.isRecording ? "#FF5252" : (root.isPlaying ? "#3DDC84" : "#888888")
                font.family: "Consolas"
                font.pixelSize: 14
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Item { Layout.fillWidth: true }

        // ──────────────────────────────────────────────
        // 3. CONTROLES DERECHOS REAPER (ESCALA GRANDE REAL)
        // ──────────────────────────────────────────────
        Row {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            // A. Cápsula de Selección (32px de alto)
            Rectangle {
                height: 32
                width: selectionRow.implicitWidth + 32
                radius: 4
                color: "#181818"
                border.color: "#303030"
                border.width: 1

                Row {
                    id: selectionRow
                    anchors.centerIn: parent
                    spacing: 24

                    Text {
                        text: "Selection:"
                        color: "#909090"
                        font.pixelSize: 11
                        font.family: "Segoe UI"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "1.1.00"
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.family: "Consolas"
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "1.1.00"
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.family: "Consolas"
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "0.0.00"
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.family: "Consolas"
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // B. Chip Compás (4/4)
            Rectangle {
                width: 44; height: 32; radius: 4
                color: "#181818"; border.color: "#303030"; border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "4/4"
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.family: "Consolas"
                    font.bold: true
                }
            }

            // C. Chip Tempo (♩ = 120 + Metrónomo)
            Rectangle {
                height: 32
                width: tempoRow.implicitWidth + 14
                radius: 4
                color: "#181818"
                border.color: "#303030"
                border.width: 1

                Row {
                    id: tempoRow
                    anchors.centerIn: parent
                    spacing: 8

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 0

                        Row {
                            spacing: 3
                            anchors.horizontalCenter: parent.horizontalCenter

                            Canvas {
                                width: 8; height: 10
                                anchors.verticalCenter: parent.verticalCenter
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.fillStyle = "#B0B0B0"
                                    ctx.beginPath()
                                    ctx.arc(2.5, 7.5, 2.2, 0, Math.PI * 2)
                                    ctx.fill()
                                    ctx.fillRect(3.8, 1, 1.2, 7)
                                }
                                Component.onCompleted: requestPaint()
                            }

                            Text {
                                text: "="
                                color: "#B0B0B0"
                                font.pixelSize: 11
                                font.family: "Segoe UI"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            text: "120"
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            font.family: "Consolas"
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }

                    // Sub-caja dividida derecha
                    Rectangle {
                        width: 18; height: 26
                        radius: 2
                        color: "transparent"
                        border.color: "#383838"
                        border.width: 1
                        anchors.verticalCenter: parent.verticalCenter

                        Column {
                            anchors.fill: parent

                            Item {
                                width: parent.width; height: 13
                                Canvas {
                                    anchors.centerIn: parent
                                    width: 12; height: 10
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.reset()
                                        ctx.strokeStyle = "#B0B0B0"
                                        ctx.lineWidth = 1.2
                                        ctx.beginPath()
                                        ctx.arc(6, 9, 5, Math.PI * 1.1, Math.PI * 1.9)
                                        ctx.stroke()
                                        ctx.beginPath()
                                        ctx.moveTo(6, 9); ctx.lineTo(10, 2)
                                        ctx.stroke()
                                    }
                                    Component.onCompleted: requestPaint()
                                }
                            }

                            Rectangle { width: parent.width; height: 1; color: "#383838" }

                            Item {
                                width: parent.width; height: 12
                                Canvas {
                                    anchors.centerIn: parent
                                    width: 8; height: 10
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.reset()
                                        ctx.fillStyle = "#B0B0B0"
                                        ctx.beginPath()
                                        ctx.arc(2.5, 7.5, 2.2, 0, Math.PI * 2)
                                        ctx.fill()
                                        ctx.fillRect(3.8, 1, 1.2, 7)
                                    }
                                    Component.onCompleted: requestPaint()
                                }
                            }
                        }
                    }
                }
            }

            // D. Chip GLOBAL OFF
            Rectangle {
                height: 32
                width: globalRow.implicitWidth + 16
                radius: 4
                color: "#181818"
                border.color: "#303030"
                border.width: 1

                Row {
                    id: globalRow
                    anchors.centerIn: parent
                    spacing: 6

                    Canvas {
                        width: 14; height: 14
                        anchors.verticalCenter: parent.verticalCenter
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.strokeStyle = "#B0B0B0"
                            ctx.fillStyle = "#B0B0B0"
                            ctx.lineWidth = 1.2
                            ctx.beginPath()
                            ctx.moveTo(2, 10); ctx.lineTo(7, 3); ctx.lineTo(12, 10)
                            ctx.stroke()
                            ctx.beginPath(); ctx.arc(2, 10, 1.5, 0, Math.PI * 2); ctx.fill()
                            ctx.beginPath(); ctx.arc(7, 3, 1.5, 0, Math.PI * 2); ctx.fill()
                            ctx.beginPath(); ctx.arc(12, 10, 1.5, 0, Math.PI * 2); ctx.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: -2
                        Text { text: "GLOBAL"; color: "#FFFFFF"; font.pixelSize: 9; font.bold: true; font.family: "Segoe UI" }
                        Text { text: "OFF"; color: "#A0A0A0"; font.pixelSize: 9; font.bold: true; font.family: "Segoe UI" }
                    }

                    Text {
                        text: "▼"
                        color: "#A0A0A0"
                        font.pixelSize: 8
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // E. Perilla Rate Dial (30x30px)
            Rectangle {
                width: 30; height: 30
                radius: 15
                color: "#222222"
                border.color: "#101010"
                border.width: 1
                anchors.verticalCenter: parent.verticalCenter
                antialiasing: true

                Rectangle {
                    width: 2
                    height: 11
                    color: "#FFFFFF"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 4
                    antialiasing: true
                }
            }

            // F. Texto Rate apilado
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                Text {
                    text: "Rate:"
                    color: "#A0A0A0"
                    font.pixelSize: 10
                    font.family: "Segoe UI"
                }
                Text {
                    text: "1.0"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                    font.family: "Consolas"
                    font.bold: true
                }
            }
        }
    }
}