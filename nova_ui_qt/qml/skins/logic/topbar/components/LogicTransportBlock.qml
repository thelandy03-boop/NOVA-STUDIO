import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 250
    implicitHeight: 70

    // Pista de botones de transporte ampliada
    Rectangle {
        anchors.centerIn: parent
        width: 245
        height: 48 // Ampliado a 48px de alto
        radius: 6
        color: "#32363F"
        border.color: "#1A1C22"
        border.width: 1
        clip: true

        Row {
            anchors.fill: parent

            // 1. Rewind
            Rectangle {
                width: 48
                height: parent.height
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/LogicIconRewind.svg")
                    width: 24
                    height: 24
                    opacity: 0.95
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 2. Play
            Rectangle {
                width: 48
                height: parent.height
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/LogicIconPlay.svg")
                    width: 26
                    height: 26
                    opacity: 0.95
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 3. Stop
            Rectangle {
                width: 48
                height: parent.height
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/LogicIconStop.svg")
                    width: 22
                    height: 22
                    opacity: 0.95
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 4. Record
            Rectangle {
                width: 48
                height: parent.height
                color: "transparent"
                Rectangle {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    radius: 10
                    color: "#E53935"
                }
            }
            Rectangle { width: 1; height: parent.height; color: "#1A1C22" }

            // 5. Loop
            Rectangle {
                width: 48
                height: parent.height
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("../icons/svg/IconLoop.svg")
                    width: 24
                    height: 24
                    opacity: 0.95
                }
            }
        }
    }
}