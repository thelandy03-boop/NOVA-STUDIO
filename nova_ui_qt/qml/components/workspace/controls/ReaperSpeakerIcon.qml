import QtQuick

Canvas {
    width: 14
    height: 14
    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.fillStyle = "#606060"
        ctx.beginPath()
        ctx.moveTo(1, 4)
        ctx.lineTo(4, 4)
        ctx.lineTo(7, 1)
        ctx.lineTo(7, 13)
        ctx.lineTo(4, 10)
        ctx.lineTo(1, 10)
        ctx.closePath()
        ctx.fill()
        ctx.strokeStyle = "#606060"
        ctx.lineWidth = 1.2
        ctx.beginPath()
        ctx.arc(8, 7, 2.5, -0.9, 0.9)
        ctx.stroke()
    }
    Component.onCompleted: requestPaint()
}