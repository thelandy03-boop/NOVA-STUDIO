import QtQuick
import QtQuick.Shapes

Shape {
    id: root
    width: 24
    height: 24

    property color color: "#E0E3EB"

    ShapePath {
        fillColor: root.color
        strokeColor: "#1A1C22"
        strokeWidth: 1.2

        PathSvg {
            // Cabezal redondeado superior estilo Logic Pro (Punta en X=12, Y=22)
            path: "M 4,4 C 4,1.8 7.6,1 12,1 C 16.4,1 20,1.8 20,4 C 20,10 16,16 12,22 C 8,16 4,10 4,4 Z"
        }
    }
}