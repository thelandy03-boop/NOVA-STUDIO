import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 210
    implicitHeight: 70

    Rectangle {
        anchors.centerIn: parent
        width: 200
        height: 48
        radius: 6
        color: "#14161B"
        border.color: "#0B0C0E"
        border.width: 1

        Column {
            anchors.centerIn: parent
            spacing: 1

            // NÚMERO GIGANTE: COMPÁS MUSICAL REAL (Coincide con el 1, 2, 3, 4, 5... de la regla)
            Text {
                id: mainBbtText
                anchors.horizontalCenter: parent.horizontalCenter
                text: (typeof AudioEngine !== "undefined" && AudioEngine.bbt !== "") 
                      ? AudioEngine.bbt 
                      : "001 : 01 : 01"
                color: "#E5E9F0"
                font.pixelSize: 22
                font.weight: Font.Bold
                font.family: "Monospace"
            }

            // TEXTO SECUNDARIO: Reloj en Segundos + Snap
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: ((typeof AudioEngine !== "undefined") ? AudioEngine.timecode : "00:00:00") + "  |  Snap: 1/16"
                color: "#818693"
                font.pixelSize: 11
                font.weight: Font.Normal
                font.family: "sans-serif"
            }
        }
    }
}