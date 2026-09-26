import QtQuick
import QtQuick.Shapes

Shape {
    width: 24
    height: 24
    asynchronous: true
    preferredRendererType: Shape.CurveRenderer

    ShapePath {
        fillColor: "#A0A5B5"
        fillRule: ShapePath.OddEvenFill
        PathSvg {
            path: "M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18M1 12a11 11 0 1 1 22 0 11 11 0 0 1-22 0 M10 10.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m7 0a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m-5 4.65c1.13 0 2.16-.45 2.9-1.19l1.2 1.22a5.83 5.83 0 0 1-8.2 0l1.2-1.22a4 4 0 0 0 2.9 1.19"
        }
    }
}
