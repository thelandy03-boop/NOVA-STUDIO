import QtQuick
import QtQuick.Shapes

Item {
    id: root
    width: 7
    height: 5
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
            PathSvg { path: "M 1 1 L 3.5 3.5 L 6 1" }
        }
    }
}
