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
                path: "M12 4.7a2 2 0 0 0-3.52-1.3L4.54 8H2a2 2 0 0 0-2 2v4c0 1.1.9 2 2 2h2.54l3.94 4.6A2 2 0 0 0 12 19.3zm-2 0v14.6l-3.94-4.6a2 2 0 0 0-1.52-.7H2v-4h2.54a2 2 0 0 0 1.52-.7z M14 9v6h2V9zm8-4v14h2V5zm-4 2v10h2V7z"
            }
        }
    }
}
