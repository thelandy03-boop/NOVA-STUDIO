import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property color iconColor: "#FFFFFF"

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
                path: "M11 13v8h2v-8h8v-2h-8V3h-2v8H3v2z"
            }
        }
    }
}