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
                path: "M19 4v3h-2v2h2v3h2V9h2V7h-2V4zm0 5V7h2v2z M1 7h14.01v2H1zm22 8H13v2h10z M3.13 17a4 4 0 1 0 0-2H1v2zM5 16a2 2 0 1 1 4 0 2 2 0 0 1-4 0"
            }
        }
    }
}