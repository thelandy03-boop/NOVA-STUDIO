import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property color iconColor: "#8F94A0"
    implicitWidth: 16
    implicitHeight: 16

    Shape {
        anchors.centerIn: parent
        width: 24
        height: 24
        scale: root.width / 24.0
        transformOrigin: Item.Center
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: root.iconColor
            strokeColor: "transparent"
            strokeWidth: 0
            fillRule: ShapePath.OddEvenFill

            PathSvg {
                path: "m14.59 12-6.3-6.3 1.42-1.4 7.7 7.7-7.7 7.7-1.42-1.4z"
            }
        }
    }
}