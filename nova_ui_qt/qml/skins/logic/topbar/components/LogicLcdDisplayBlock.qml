import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 210
    implicitHeight: 70

    Rectangle {
        anchors.centerIn: parent
        width: 200
        height: 48 // Pantalla LCD prominente de 48px
        radius: 6
        color: "#14161B"
        border.color: "#0B0C0E"
        border.width: 1

        Column {
            anchors.centerIn: parent
            spacing: 0

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "01:01:000"
                color: "#E5E9F0"
                font.pixelSize: 24 // Número grande estilo pantalla DAW
                font.weight: Font.DemiBold
                font.family: "sans-serif"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "01:01:000  |  Snap: 1/16"
                color: "#818693"
                font.pixelSize: 11
                font.weight: Font.Normal
                font.family: "sans-serif"
            }
        }
    }
}