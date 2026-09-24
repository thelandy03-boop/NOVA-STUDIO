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

            PathSvg {
                path: "M8.3 2.3 1.58 9l6.7 6.7 1.42-1.4L5.4 10H14c4.12 0 6 2.59 6.03 5a4.6 4.6 0 0 1-1.42 3.4c-.96.92-2.46 1.6-4.61 1.6v2c2.59 0 4.61-.82 6-2.15a6.6 6.6 0 0 0 2.03-4.86C22 11.41 19.15 8 14 8H5.41l4.3-4.3z"
            }
        }
    }
}