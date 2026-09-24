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
                path: "M3 10V2h2v8zm10-8v4h-2V2zm0 11v9h-2v-9H9v-2h6v2zm6-11v12h2V2zM3 18v4h2v-4h2v-2H1v2zm12-8H9V8h6zm8 9h-6v-2h6zM1 15h6v-2H1zm22 7h-6v-2h6z"
            }
        }
    }
}