import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    width: 24
    height: 24

    property color color: "#D0D3D9"

    ShapePath {
        fillColor: root.color
        strokeColor: "transparent"

        PathSvg {
            // Flecha desplegable (chevron) más amplia y centrada
            path: "M6 9l6 6 6-6H6z"
        }
    }
}