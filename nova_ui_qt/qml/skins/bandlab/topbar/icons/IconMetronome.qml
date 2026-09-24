import QtQuick
import QtQuick.Shapes

Item {
    id: root
    width: 13
    height: 13
    property color color: "#A0A5B5"

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: 1.2
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap
            PathSvg { path: "M 6.5 1 L 11.5 12 H 1.5 Z M 6.5 1 L 9.5 7.5" }
        }
        ShapePath {
            fillColor: root.color
            strokeWidth: 0
            PathSvg { path: "M 8.5 6.5 A 1.2 1.2 0 1 0 10.9 6.5 A 1.2 1.2 0 1 0 8.5 6.5 Z" }
        }
    }
}
