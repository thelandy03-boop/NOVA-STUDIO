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
            path: "M8 5v14l11-7z"
        }
    }
}