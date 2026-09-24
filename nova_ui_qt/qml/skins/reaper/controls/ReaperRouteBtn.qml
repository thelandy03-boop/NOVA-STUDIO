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
                var h = height
                var w = width
                ctx.fillStyle = "#00D8B4"
                ctx.beginPath(); ctx.moveTo(0, h); ctx.lineTo(5, 0); ctx.lineTo(9, 0); ctx.lineTo(4, h); ctx.fill()
                ctx.fillStyle = "#6B5A40"
                ctx.beginPath(); ctx.moveTo(5, h); ctx.lineTo(10, 0); ctx.lineTo(14, 0); ctx.lineTo(9, h); ctx.fill()
                ctx.fillStyle = "#33414C"
                ctx.beginPath(); ctx.moveTo(10, h); ctx.lineTo(15, 0); ctx.lineTo(19, 0); ctx.lineTo(14, h); ctx.fill()
            }
            Component.onCompleted: requestPaint()
        }
    }
}