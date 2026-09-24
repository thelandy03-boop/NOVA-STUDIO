import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property color iconColor: "#FFB000"
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
                path: "M19 21H5v-2h14z m-0.47-13.66 6.42-1.75-2.67 10.65-.2.76H4.99L2.1 5.6l6.42 1.75L12 1.54zm-6 2.33L4.89 8.42 6.54 15h10.93l1.64-6.58-4.58 1.25L12 5.43z"
            }
        }
    }
}