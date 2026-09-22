import QtQuick

ReaperBaseButton {
    id: root

    implicitWidth: 22
    implicitHeight: 28

    property bool fxActive: true
    signal fxClicked()
    signal powerToggled()

    Rectangle {
        anchors.fill: parent
        radius: 2
        color: "#111111"
        border.color: "#111111"
        border.width: 1

        // Bisel 3D exterior
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: "transparent"
            border.color: "#3F3F3F"
            border.width: 1
            radius: 1
        }

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // 1. PARTE SUPERIOR: TEXTO "FX"
            Rectangle {
                width: parent.width
                height: 13
                color: "#2A2A2A"
                radius: 1

                Text {
                    anchors.centerIn: parent
                    text: "FX"
                    color: "#D0D0D0"
                    font.pixelSize: 8
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.fxClicked()
                }
            }

            // Divisor
            Rectangle {
                width: parent.width
                height: 1
                color: "#111111"
            }

            // 2. PARTE INFERIOR: BOTÓN POWER
            Rectangle {
                width: parent.width
                height: 12
                color: root.fxActive ? "#555555" : "#222222"
                radius: 1

                Canvas {
                    id: pwrIco
                    anchors.centerIn: parent
                    width: 8
                    height: 8

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = root.fxActive ? "#181818" : "#777777"
                        ctx.lineWidth = 1.3
                        ctx.lineCap = "round"

                        // Arco Power
                        ctx.beginPath()
                        ctx.arc(4, 4, 3, -Math.PI * 0.35, Math.PI * 1.35, false)
                        ctx.stroke()

                        // Línea vertical
                        ctx.beginPath()
                        ctx.moveTo(4, 1)
                        ctx.lineTo(4, 4)
                        ctx.stroke()
                    }
                    Component.onCompleted: requestPaint()
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.fxActive = !root.fxActive
                        pwrIco.requestPaint()
                        root.powerToggled()
                    }
                }
            }
        }
    }
}