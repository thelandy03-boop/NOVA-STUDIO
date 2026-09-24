import QtQuick
import QtQuick.Shapes

Item {
    id: root

    implicitWidth: 16
    implicitHeight: 16
    width: implicitWidth
    height: implicitHeight

    property color color: "#A0A5B5"

    Shape {
        anchors.centerIn: parent
        width: 24
        height: 24
        scale: Math.min(root.width / 24.0, root.height / 24.0)
        transformOrigin: Item.Center
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: root.color
            strokeWidth: 0
            fillRule: ShapePath.OddEvenFill

            PathSvg {
                path: "M3 18V2H1v16a4 4 0 0 0 4 4h14a4 4 0 0 0 4-4V2h-2v16a2 2 0 0 1-2 2h-3v-8h1V2h-3.5v10h.5v8h-4v-8h.5V2H7v10h1v8H5a2 2 0 0 1-2-2"
            }
        }
    }
}