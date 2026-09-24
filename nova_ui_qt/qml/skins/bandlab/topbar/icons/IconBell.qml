import QtQuick
import QtQuick.Shapes

Item {
    id: root
    width: 13
    height: 14
    property color color: "#A0A5B5"

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: 1.2
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: "M 6.5 1.5 A 3 3 0 0 0 3.5 4.5 V 8 L 2 9.5 H 11 L 9.5 8 V 4.5 A 3 3 0 0 0 6.5 1.5 Z" }
        }
        ShapePath {
            fillColor: root.color
            strokeWidth: 0
            PathSvg { path: "M 5 11 A 1.5 1.5 0 0 0 8 11 Z" }
        }
    }
}
