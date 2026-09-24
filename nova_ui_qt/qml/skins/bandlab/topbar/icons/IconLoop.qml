import QtQuick
import QtQuick.Shapes

Item {
    id: root
    width: 13
    height: 12
    property color color: "#A0A5B5"

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: 1.3
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: "M 3 2.5 H 9.5 A 2 2 0 0 1 11.5 4.5 V 5 M 9.5 1 L 11.5 2.5 L 9.5 4 M 10 9.5 H 3.5 A 2 2 0 0 1 1.5 7.5 V 7 M 3.5 11 L 1.5 9.5 L 3.5 8" }
        }
    }
}
