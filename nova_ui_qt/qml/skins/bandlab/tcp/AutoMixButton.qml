import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes

// Equivale a: mix-editor-auto-mix > auto-mix-button (1:1 con BandLab)
Rectangle {
    id: root
    implicitHeight: 38
    Layout.fillWidth: true

    color: hit.pressed ? "#161822" : (hit.containsMouse ? "#12141F" : "#0E0F16")
    border.color: "#1B1D2A"
    border.width: 1

    signal clicked()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 8

        // ==========================================
        // 1. ICONO AUTOMIX (SVG Exacto 1:1 BandLab en GPU)
        // ==========================================
        Item {
            width: 16; height: 16
            Layout.alignment: Qt.AlignVCenter

            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                scale: 16 / 24
                transformOrigin: Item.Center
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    fillColor: hit.containsMouse ? "#FFFFFF" : "#A0A4B0"
                    strokeWidth: 0
                    fillRule: ShapePath.OddEvenFill
                    PathSvg { path: "M19 4v3h-2v2h2v3h2V9h2V7h-2V4zm0 5V7h2v2z" }
                }
                ShapePath {
                    fillColor: hit.containsMouse ? "#FFFFFF" : "#A0A4B0"
                    strokeWidth: 0
                    PathSvg { path: "M1 7h14.01v2H1zm22 8H13v2h10z" }
                }
                ShapePath {
                    fillColor: hit.containsMouse ? "#FFFFFF" : "#A0A4B0"
                    strokeWidth: 0
                    fillRule: ShapePath.OddEvenFill
                    PathSvg { path: "M3.13 17a4 4 0 1 0 0-2H1v2zM5 16a2 2 0 1 1 4 0 2 2 0 0 1-4 0" }
                }
            }
        }

        // ==========================================
        // 2. TEXTO AUTOMIX
        // ==========================================
        Text {
            text: "AutoMix"
            color: "#FFFFFF"
            font.pixelSize: 12
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

        // ==========================================
        // 3. BADGE IA (ds-badge-color-blue)
        // ==========================================
        Rectangle {
            width: 18; height: 14
            radius: 3
            color: "#1D68FB"
            Layout.alignment: Qt.AlignVCenter

            Text {
                anchors.centerIn: parent
                text: "IA"
                color: "#FFFFFF"
                font.pixelSize: 9
                font.bold: true
            }
        }

        // ==========================================
        // 4. CORONA CALADA (color-membership 1:1 BandLab)
        // ==========================================
        Item {
            width: 16; height: 16
            Layout.alignment: Qt.AlignVCenter

            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                scale: 16 / 24
                transformOrigin: Item.Center
                preferredRendererType: Shape.CurveRenderer

                // Base
                ShapePath {
                    fillColor: "#FFB000"
                    strokeWidth: 0
                    PathSvg { path: "M19 21H5v-2h14z" }
                }

                // Corona calada interior (OddEvenFill ahueca el centro)
                ShapePath {
                    fillColor: "#FFB000"
                    strokeWidth: 0
                    fillRule: ShapePath.OddEvenFill
                    PathSvg { path: "m15.47 7.34 6.42-1.75-2.67 10.65-.2.76H4.99L2.1 5.6l6.42 1.75L12 1.54zm-6 2.33L4.89 8.42 6.54 15h10.93l1.64-6.58-4.58 1.25L12 5.43z" }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // ==========================================
        // 5. CHEVRON > (ds-glyphs-secondary 1:1 BandLab)
        // ==========================================
        Item {
            width: 16; height: 16
            Layout.alignment: Qt.AlignVCenter

            Shape {
                anchors.centerIn: parent
                width: 24; height: 24
                scale: 16 / 24
                transformOrigin: Item.Center
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    fillColor: hit.containsMouse ? "#FFFFFF" : "#6C7080"
                    strokeWidth: 0
                    fillRule: ShapePath.OddEvenFill
                    PathSvg { path: "m14.59 12-6.3-6.3 1.42-1.4 7.7 7.7-7.7 7.7-1.42-1.4z" }
                }
            }
        }
    }

    MouseArea {
        id: hit
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}