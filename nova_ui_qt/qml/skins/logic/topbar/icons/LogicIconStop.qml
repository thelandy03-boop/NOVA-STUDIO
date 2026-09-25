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
            path: "M6 6h12v12H6z"
        }
    }
}