import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property color iconColor: "#FFD54F"
    implicitWidth: 16
    implicitHeight: 16

    Shape {
        anchors.centerIn: parent
        width: 24; height: 24
        preferredRendererType: Shape.CurveRenderer
        transform: Scale { xScale: root.width/24; yScale: root.height/24 }

        ShapePath {
            fillColor: root.iconColor
            strokeWidth: 0
            fillRule: ShapePath.WindingFill
            PathSvg {
                path: "M12 14c1.66 0 3-1.34 3-3V5c0-1.66-1.34-3-3-3S9 3.34 9 5v6c0 1.66 1.34 3 3 3zm5-3c0 2.76-2.24 5-5 5s-5-2.24-5-5H5c0 3.53 2.61 6.43 6 6.92V21h2v-3.08c3.39-.49 6-3.39 6-6.92h-2z"
            }
        }
    }
}