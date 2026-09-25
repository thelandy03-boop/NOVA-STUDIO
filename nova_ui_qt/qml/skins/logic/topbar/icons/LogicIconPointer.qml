import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    width: 24
    height: 24

    property color color: "#FFFFFF"

    ShapePath {
        fillColor: root.color
        strokeColor: "transparent"

        PathSvg {
            // Coordenadas con balance óptico perfecto
            path: "M7.5 5v13.5l3.5-3.5 2.5 5.5 2.2-1-2.5-5.5h5z"
        }
    }
}