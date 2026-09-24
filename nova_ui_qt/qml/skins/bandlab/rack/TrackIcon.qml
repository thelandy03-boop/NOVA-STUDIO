import QtQuick

Item {
    id: root

    // Tipos: "autopitch", "fx", "edit", "feather", "sounds", "rhythm", "assistant", "note"
    property string iconType: "fx"
    property color iconColor: "#8F94A0"

    implicitWidth: 16
    implicitHeight: 16

    onIconTypeChanged: canvas.requestPaint()
    onIconColorChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.Image

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.strokeStyle = root.iconColor;
            ctx.fillStyle = root.iconColor;
            ctx.lineWidth = 1.3;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";

            // Cálculo dinámico de centro y radio seguro para evitar recortes
            var cx = width / 2;
            var cy = height / 2;
            var r = (Math.min(width, height) / 2) - 1.5;

            if (root.iconType === "autopitch") {
                // Ondas
                ctx.beginPath();
                ctx.moveTo(2, 11); ctx.bezierCurveTo(4, 11, 5, 8, 8, 8); ctx.bezierCurveTo(11, 8, 12, 5, 14, 5);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(2, 7); ctx.bezierCurveTo(4, 7, 5, 4, 8, 4); ctx.bezierCurveTo(11, 4, 12, 1, 14, 1);
                ctx.stroke();

            } else if (root.iconType === "fx") {
                // Fx
                ctx.font = "italic bold 11px sans-serif";
                ctx.fillText("Fx", 1, 12);

            } else if (root.iconType === "edit") {
                // Lápiz / Editor
                ctx.beginPath();
                ctx.moveTo(11, 2); ctx.lineTo(14, 5); ctx.lineTo(5, 14); ctx.lineTo(2, 14); ctx.lineTo(2, 11); ctx.closePath();
                ctx.stroke();

            } else if (root.iconType === "feather") {
                // Pluma / Letras
                ctx.beginPath();
                ctx.moveTo(13, 2);
                ctx.quadraticCurveTo(4, 4, 3, 14);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(8, 7); ctx.lineTo(3, 14);
                ctx.stroke();

            } else if (root.iconType === "sounds") {
                // BandLab Sounds (Círculo + Nota centrado)
                ctx.beginPath();
                ctx.arc(cx, cy, r, 0, Math.PI * 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.arc(cx - 1.5, cy + 2.0, 1.2, 0, Math.PI * 2);
                ctx.fill();

                ctx.beginPath();
                ctx.moveTo(cx - 0.3, cy + 2.0);
                ctx.lineTo(cx - 0.3, cy - 2.5);
                ctx.lineTo(cx + 2.5, cy - 2.5);
                ctx.lineTo(cx + 2.5, cy - 0.5);
                ctx.stroke();

            } else if (root.iconType === "rhythm") {
                // Ritmos (Círculo + Pulso centrado)
                ctx.beginPath();
                ctx.arc(cx, cy, r, 0, Math.PI * 2);
                ctx.stroke();

                ctx.beginPath();
                ctx.moveTo(cx - r + 2.0, cy);
                ctx.lineTo(cx - 2.0, cy - 2.5);
                ctx.lineTo(cx, cy + 3.0);
                ctx.lineTo(cx + 2.0, cy - 2.0);
                ctx.lineTo(cx + r - 2.0, cy);
                ctx.stroke();

            } else if (root.iconType === "assistant") {
                // Asistente (Carita Sonriente centrada)
                ctx.beginPath();
                ctx.arc(cx, cy, r, 0, Math.PI * 2);
                ctx.stroke();

                // Ojos
                ctx.fillRect(cx - 2.8, cy - 2.5, 1.5, 1.5);
                ctx.fillRect(cx + 1.3, cy - 2.5, 1.5, 1.5);

                // Sonrisa
                ctx.beginPath();
                ctx.arc(cx, cy + 0.5, r * 0.5, 0.2, Math.PI - 0.2);
                ctx.stroke();
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }
}