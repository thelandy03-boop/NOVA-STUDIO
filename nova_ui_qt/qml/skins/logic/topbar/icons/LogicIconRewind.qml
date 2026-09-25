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
            path: "M11.5 12l8.5 6V6l-8.5 6zm-1.5 6V6H8v12h2z"
        }
    }
}