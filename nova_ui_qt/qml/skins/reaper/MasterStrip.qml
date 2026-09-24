import QtQuick
import QtQuick.Controls
import "controls"

Rectangle {
    id: root
    width: 124
    color: "#242424"

    property real faderValue: 0.75
    property bool muted: false
    property bool solo: false
    property bool mono: false
    property bool fxActive: true

    Column {
        anchors.fill: parent
        spacing: 0

        // ─────────────────────────────────────────
        // 1. CABECERA (center + Knob)
        // ─────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 44
            color: "#2C2C2C"

            Column {
                anchors.centerIn: parent
                spacing: 1

                Text {
                    text: "center"
                    color: "#9A9A9A"
                    font.pixelSize: 9
                    font.family: "Segoe UI"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                ReaperKnob {
                    value: 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // ─────────────────────────────────────────
        // 2. CUERPO PRINCIPAL
        // ─────────────────────────────────────────
        Item {
            width: parent.width
            height: parent.height - 44 - 20 // Resta header y badge

            // ── COLUMNA DERECHA DE BOTONES ESTILO REAPER ──
            Column {
                id: rightCol
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top: parent.top
                anchors.topMargin: 4
                spacing: 2
                width: 22

                // 1. Botón MONO (Icono + Texto debajo)
                Column {
                    width: 22
                    spacing: 1

                    ReaperBaseButton {
                        implicitWidth: 22
                        implicitHeight: 18

                        Rectangle {
                            anchors.fill: parent
                            radius: 2
                            color: parent.active ? "#4A4A4A" : "#262626"
                            border.color: "#111111"
                            border.width: 1

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 1
                                radius: 1
                                color: "transparent"
                                border.color: "#3F3F3F"
                                border.width: 1
                            }

                            // Icono dos círculos entrelazados (MONO)
                            Item {
                                width: 14; height: 10
                                anchors.centerIn: parent
                                Rectangle {
                                    width: 9; height: 9; radius: 4.5
                                    color: "transparent"
                                    border.color: parent.parent.parent.parent.active ? "#FFFFFF" : "#808080"
                                    border.width: 1.2
                                    x: 0; y: 0.5
                                }
                                Rectangle {
                                    width: 9; height: 9; radius: 4.5
                                    color: "transparent"
                                    border.color: parent.parent.parent.parent.active ? "#FFFFFF" : "#808080"
                                    border.width: 1.2
                                    x: 5; y: 0.5
                                }
                            }
                        }
                        onToggled: root.mono = !root.mono
                    }

                    Text {
                        text: "MONO"
                        color: "#808080"
                        font.pixelSize: 7
                        font.bold: true
                        font.family: "Segoe UI"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                // 2. Mute
                ReaperTinyBtn {
                    label: "M"; labelColor: "#E55B5B"; activeColor: "#C62828"
                    active: root.muted; onTapped: root.muted = !root.muted
                }

                // 3. Solo
                ReaperTinyBtn {
                    label: "S"; labelColor: "#FFD54F"; activeColor: "#F9A825"
                    active: root.solo; onTapped: root.solo = !root.solo
                }

                // 4. Route (Icono + Texto "ROUTE" debajo)
                Column {
                    width: 22
                    spacing: 1

                    ReaperRouteBtn {}

                    Text {
                        text: "ROUTE"
                        color: "#808080"
                        font.pixelSize: 7
                        font.bold: true
                        font.family: "Segoe UI"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                // 5. FX Split / Power
                ReaperFxBtn {
                    fxActive: root.fxActive
                    onPowerToggled: root.fxActive = !root.fxActive
                }

                // 6. Automation (Icono + Texto "TRIM" debajo)
                Column {
                    width: 22
                    spacing: 1

                    ReaperNodesBtn {}

                    Text {
                        text: "TRIM"
                        color: "#808080"
                        font.pixelSize: 7
                        font.bold: true
                        font.family: "Segoe UI"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                // 7. Info button
                ReaperTinyBtn {
                    label: "i"
                    labelColor: "#808080"
                }
            }

            // ── CAJA OSCURA EMPOTRADA (Meters + Escala + Fader) ──
            Rectangle {
                id: darkBox
                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.right: rightCol.left
                anchors.rightMargin: 4
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
                color: "#161616"
                border.color: "#101010"
                border.width: 1

                // Lectura superior 0.00dB
                Text {
                    id: dbReadout
                    text: "0.00dB"
                    color: "#7A7A7A"
                    font.pixelSize: 9
                    font.family: "Segoe UI"
                    anchors.top: parent.top
                    anchors.topMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Cabecera -inf -inf
                Row {
                    id: infHeader
                    anchors.top: dbReadout.bottom
                    anchors.topMargin: 2
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 12

                    Text { text: "-inf"; color: "#555555"; font.pixelSize: 8; font.family: "Consolas" }
                    Text { text: "-inf"; color: "#555555"; font.pixelSize: 8; font.family: "Consolas" }
                }

                // Zona Central: Escala + Fader Metálico
                Item {
                    id: innerArea
                    anchors.top: infHeader.bottom
                    anchors.topMargin: 2
                    anchors.bottom: rmsBar.top
                    anchors.bottomMargin: 2
                    anchors.left: parent.left
                    anchors.right: parent.right

                    // Escala numérica triple estilo REAPER
                    Item {
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.right: fader.left
                        anchors.rightMargin: 2
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom

                        Repeater {
                            model: [
                                { l: "12", m: "", r: "12" },
                                { l: "6",  m: "-6-", r: "6" },
                                { l: "0-", m: "", r: "0-" },
                                { l: "6-", m: "-18-", r: "6-" },
                                { l: "12-", m: "", r: "12-" },
                                { l: "18-", m: "-30-", r: "18-" },
                                { l: "24-", m: "", r: "24-" },
                                { l: "30-", m: "-42-", r: "30-" },
                                { l: "36-", m: "", r: "36-" },
                                { l: "42-", m: "-54-", r: "42-" }
                            ]
                            delegate: Item {
                                required property var modelData
                                required property int index
                                width: parent.width
                                height: parent.height / 10
                                y: index * height

                                Text {
                                    text: modelData.l
                                    color: "#444444"
                                    font.pixelSize: 7
                                    font.family: "Consolas"
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: modelData.m
                                    color: "#444444"
                                    font.pixelSize: 7
                                    font.family: "Consolas"
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: modelData.r
                                    color: "#444444"
                                    font.pixelSize: 7
                                    font.family: "Consolas"
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }

                    // Fader Metálico
                    ReaperFader {
                        id: fader
                        width: 22
                        anchors.right: parent.right
                        anchors.rightMargin: 2
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        value: root.faderValue
                        onMoved: (v) => root.faderValue = v
                    }
                }

                // Rótulo inferior RMS -inf en verde REAPER
                Row {
                    id: rmsBar
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8

                    Text {
                        text: "RMS"
                        color: "#3DDC84"
                        font.pixelSize: 8
                        font.bold: true
                        font.family: "Segoe UI"
                    }

                    Text {
                        text: "-inf"
                        color: "#3DDC84"
                        font.pixelSize: 8
                        font.family: "Consolas"
                    }
                }
            }
        }

        // ─────────────────────────────────────────
        // 3. BADGE MASTER INFERIOR
        // ─────────────────────────────────────────
        Rectangle {
            width: parent.width
            height: 20
            color: "#3A3A3A"

            Text {
                anchors.centerIn: parent
                text: "MASTER"
                color: "#D6D6D6"
                font.pixelSize: 10
                font.bold: true
                font.family: "Segoe UI"
            }
        }
    }
}