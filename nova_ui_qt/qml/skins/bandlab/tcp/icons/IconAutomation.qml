import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property color iconColor: "#A0A4B0"

    implicitWidth: 16
    implicitHeight: 16

    Shape {
        anchors.centerIn: parent
        width: 24
        height: 24

        // Escala desde el centro exacto para evitar desplazamientos
        scale: root.width / 24.0
        transformOrigin: Item.Center
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            fillColor: root.iconColor
            strokeColor: "transparent"
            strokeWidth: 0

            // CRÍTICO: OddEvenFill cala los agujeros interiores de los nodos (evenodd)
            fillRule: ShapePath.OddEvenFill

            PathSvg {
                path: "M19.87 6a4 4 0 1 0-7 3.5L9.9 13.48Q9.04 13.02 8 13a4 4 0 0 0-3.87 3H1v2h3.13a4 4 0 1 0 7.24-3.16l3.1-4.14a4 4 0 0 0 5.4-2.7H23V6zM16 5a2 2 0 1 0 0 4 2 2 0 0 0 0-4M8 15a2 2 0 1 0 0 4 2 2 0 0 0 0-4"
            }
        }
    }
}