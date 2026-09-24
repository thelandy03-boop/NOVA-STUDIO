import QtQuick

ReaperBaseButton {
    id: root

    implicitWidth: 22
    implicitHeight: 18

    Rectangle {
        anchors.fill: parent
        radius: 2
        color: "#262626"
        border.color: "#111111"
        border.width: 1

        // Bisel 3D interior
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 1
            color: "transparent"
            border.color: "#3F3F3F"
            border.width: 1
        }

        Canvas {
            anchors.fill: parent
            anchors.margins: 2
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = "#808080"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(2, 12)
                ctx.lineTo(9, 3)
                ctx.lineTo(16, 12)
                ctx.stroke()
                ctx.fillStyle = "#D0D0D0"
                ctx.beginPath(); ctx.arc(2, 12, 1.5, 0, 6.3); ctx.fill()
                ctx.beginPath(); ctx.arc(9, 3, 1.5, 0, 6.3); ctx.fill()
                ctx.beginPath(); ctx.arc(16, 12, 1.5, 0, 6.3); ctx.fill()
            }
            Component.onCompleted: requestPaint()
        }
    }
}