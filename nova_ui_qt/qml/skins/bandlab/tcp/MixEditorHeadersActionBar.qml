import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes

// BandLab: <div class="mix-editor-headers-actionbar">
Rectangle {
    id: root

    implicitHeight: 48
    Layout.fillWidth: true
    color: "transparent"

    signal addTrackClicked()
    signal automationToggled()
    signal stretchToggled()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 8

        // ── ds-button: + Añadir pista ───────────────────
        Rectangle {
            id: addBtn
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: addRow.implicitWidth + 28
            implicitHeight: 34
            radius: 17

            color: addHit.pressed ? "#1A1B22"
                 : (addHit.containsMouse ? "#1C1C24" : "#14151C")
            border.width: 1
            border.color: addHit.containsMouse ? "#2E3240" : "#252830"

            Row {
                id: addRow
                anchors.centerIn: parent
                spacing: 8

                // SVG 16×16, path viewBox 24×24
                Item {
                    width: 16
                    height: 16
                    anchors.verticalCenter: parent.verticalCenter

                    Shape {
                        width: 24
                        height: 24
                        anchors.centerIn: parent
                        scale: 16 / 24
                        transformOrigin: Item.Center
                        preferredRendererType: Shape.CurveRenderer

                        ShapePath {
                            fillColor: "#FFFFFF"
                            strokeWidth: 0
                            PathSvg {
                                path: "M11 13v8h2v-8h8v-2h-8V3h-2v8H3v2z"
                            }
                        }
                    }
                }

                Text {
                    text: "Añadir pista"
                    color: "#F2F2F4"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: addHit
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.addTrackClicked()
            }
        }

        Item { Layout.fillWidth: true }

        // ── derecha: flex gap 8px ─────────────────────
        Row {
            spacing: 8
            Layout.alignment: Qt.AlignVCenter

            // Automation toggle (nodos)
            Rectangle {
                width: 34
                height: 34
                radius: 17
                color: autoHit.pressed ? "#1A1A1F"
                     : (autoHit.containsMouse ? "#1C1C22" : "#141416")
                border.width: 1
                border.color: "#2A2A30"

                // Path exacto BandLab, evenodd, escala 16/24
                Item {
                    anchors.centerIn: parent
                    width: 16
                    height: 16

                    Shape {
                        width: 24
                        height: 24
                        anchors.centerIn: parent
                        scale: 16 / 24
                        transformOrigin: Item.Center
                        preferredRendererType: Shape.CurveRenderer

                        ShapePath {
                            fillColor: autoHit.containsMouse ? "#FFFFFF" : "#A0A4B0"
                            strokeWidth: 0
                            fillRule: ShapePath.OddEvenFill
                            PathSvg {
                                path: "M19.87 6a4 4 0 1 0-7 3.5L9.9 13.48Q9.04 13.02 8 13a4 4 0 0 0-3.87 3H1v2h3.13a4 4 0 1 0 7.24-3.16l3.1-4.14a4 4 0 0 0 5.4-2.7H23V6zM16 5a2 2 0 1 0 0 4 2 2 0 0 0 0-4M8 15a2 2 0 1 0 0 4 2 2 0 0 0 0-4"
                            }
                        }
                    }
                }

                MouseArea {
                    id: autoHit
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.automationToggled(true)
                }
            }

            // Botón 2: stretch de pasaje
            Rectangle {
                width: 32
                height: 32
                radius: 16
                color: strHit.pressed ? "#161722" : (strHit.containsMouse ? "#222534" : "#181A24")
                border.color: strHit.containsMouse ? "#323648" : "#222432"
                border.width: 1

                Item {
                    anchors.centerIn: parent
                    width: 16
                    height: 16

                    Shape {
                        width: 24
                        height: 24
                        anchors.centerIn: parent
                        scale: 16 / 24
                        transformOrigin: Item.Center
                        preferredRendererType: Shape.CurveRenderer

                        ShapePath {
                            fillColor: strHit.containsMouse ? "#FFFFFF" : "#A0A4B0"
                            strokeColor: "transparent"
                            strokeWidth: 0
                            PathSvg {
                                path: "M2.6 1.2C4.89 2.9 8.23 4 12 4s7.11-1.1 9.4-2.8l1.2 1.6C19.9 4.8 16.13 6 12 6S4.1 4.8 1.4 2.8zM19.59 13l-1.8 1.8 1.42 1.4 4.2-4.2-4.2-4.2-1.42 1.4 1.8 1.8H16v2zM4.41 11l1.8-1.8-1.42-1.4L.6 12l4.2 4.2 1.42-1.4L4.4 13H8v-2zM12 20c3.77 0 7.11 1.1 9.4 2.8l1.2-1.6c-2.7-2-6.47-3.2-10.6-3.2s-7.9 1.2-10.6 3.2l1.2 1.6C4.89 21.1 8.23 20 12 20m-2-7h4v-2h-4z"
                            }
                        }
                    }
                }

                MouseArea {
                    id: strHit
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.stretchToggled(true)
                }
            }
        }
    }
}