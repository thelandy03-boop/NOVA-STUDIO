import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 135
    implicitHeight: 70

    // Separador vertical derecho
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#0E0F13"
    }

    Column {
        anchors.centerIn: parent
        spacing: 4

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5

            Text {
                text: "File / Edit"
                color: "#D0D3D9"
                font.pixelSize: 11
                font.weight: Font.Medium
                font.family: "sans-serif"
            }

            Image {
                source: Qt.resolvedUrl("../icons/svg/LogicIconChevron.svg")
                width: 12; height: 12
                anchors.verticalCenter: parent.verticalCenter
                opacity: 0.90
            }
        }

        Rectangle {
            width: 114
            height: 34
            radius: 5
            color: "#32363F"
            border.color: "#1A1C22"
            border.width: 1
            clip: true

            Row {
                anchors.fill: parent

                // 1. Puntero (Selección)
                Rectangle {
                    width: 37; height: parent.height
                    color: "#1A1C22"
                    Image {
                        anchors.centerIn: parent
                        source: Qt.resolvedUrl("../icons/svg/LogicIconPointer.svg")
                        width: 22; height: 22
                    }
                }
                Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

                // 2. Lápiz (Edición / Dibujo)
                Rectangle {
                    width: 37; height: parent.height
                    color: "transparent"
                    Image {
                        anchors.centerIn: parent
                        source: Qt.resolvedUrl("../icons/svg/LogicIconPencil.svg")
                        width: 22; height: 22
                        opacity: 0.85
                    }
                }
                Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

                // 3. Tijeras ✂️ (Cortar Clip en el Playhead)
                Rectangle {
                    id: scissorsBtn
                    width: 37; height: parent.height
                    color: scissorsMouse.containsPress ? "#1E222B" : (scissorsMouse.containsMouse ? "#3D4350" : "transparent")

                    Image {
                        anchors.centerIn: parent
                        source: Qt.resolvedUrl("../icons/svg/LogicIconScissors.svg")
                        width: 22; height: 22
                        opacity: scissorsMouse.containsMouse ? 1.0 : 0.85
                        scale: scissorsMouse.containsPress ? 0.90 : 1.0

                        Behavior on scale { NumberAnimation { duration: 80 } }
                    }

                    MouseArea {
                        id: scissorsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            if (AudioEngine.splitAtPlayhead()) {
                                console.log("✂️ [LogicToolBlock] Clip cortado exitosamente con las tijeras.")
                            }
                        }
                    }
                }
            }
        }
    }
}