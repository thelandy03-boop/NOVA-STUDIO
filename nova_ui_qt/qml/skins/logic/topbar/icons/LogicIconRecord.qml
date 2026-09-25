import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    width: 24
    height: 24
    property color color: "#E53935"

    ShapePath {
        fillColor: root.color
        strokeColor: "transparent"
        PathSvg {
            path: "M12 5a7 7 0 1 1 0 14 7 7 0 0 1 0-14z"
        }
    }
}