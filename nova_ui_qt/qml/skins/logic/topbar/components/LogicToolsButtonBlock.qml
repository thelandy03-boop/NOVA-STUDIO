import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 54
    implicitHeight: 70

    // Sin separador derecho (se conserva únicamente el separador izquierdo que viene de File / Edit)

    Column {
        anchors.centerIn: parent
        spacing: 4

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Tools"
            color: "#D0D3D9"
            font.pixelSize: 11
            font.weight: Font.Medium
            font.family: "sans-serif"
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 38
            height: 34
            radius: 5
            color: "#32363F"
            border.color: "#1A1C22"
            border.width: 1

            Image {
                anchors.centerIn: parent
                source: Qt.resolvedUrl("../icons/svg/LogicIconWrench.svg")
                width: 20
                height: 20
                opacity: 0.85
            }
        }
    }
}