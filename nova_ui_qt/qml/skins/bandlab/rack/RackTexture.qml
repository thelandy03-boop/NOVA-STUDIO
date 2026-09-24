import QtQuick

Item {
    id: root
    anchors.fill: parent

    // Propiedades personalizables para reutilizar en Cards, Headers, Faders, etc.
    property color baseColor: "#14151B"
    property color highlightColor: "#FFFFFF"
    property real highlightOpacity: 0.08
    property real noiseOpacity: 0.05
    property bool showTopBevel: true
    property bool showBottomShadow: true
    property real cornerRadius: 0

    // 1. Fondo Base con Degradado Físico Curvado
    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(root.baseColor, 1.18) }
            GradientStop { position: 0.4; color: root.baseColor }
            GradientStop { position: 1.0; color: Qt.darker(root.baseColor, 1.25) }
        }
    }

    // 2. Textura de Micro-Grano de Aluminio (Anodized Grain)
    Canvas {
        id: noiseCanvas
        anchors.fill: parent
        opacity: root.noiseOpacity
        renderTarget: Canvas.Image

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            var imgData = ctx.createImageData(width, height);
            var data = imgData.data;
            // Generador de ruido táctil fino
            for (var i = 0; i < data.length; i += 4) {
                var grain = Math.floor(Math.random() * 255);
                data[i]     = grain; // R
                data[i + 1] = grain; // G
                data[i + 2] = grain; // B
                data[i + 3] = 45;    // Alpha del ruido
            }
            ctx.putImageData(imgData, 0, 0);
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // 3. Bisel Superior de Luz Metálica (Top Bevel Line)
    Rectangle {
        visible: root.showTopBevel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: root.highlightColor
        opacity: root.highlightOpacity
        z: 2
    }

    // 4. Sombra Inferior de Profundidad (Bottom Drop Shadow)
    Rectangle {
        visible: root.showBottomShadow
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#000000"
        opacity: 0.65
        z: 2
    }
}