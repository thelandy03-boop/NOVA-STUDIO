import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 260
    implicitHeight: 44

    // Botón "+ Add Track" directo
    Rectangle {
        id: btnAddTrack
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        radius: 5
        color: mouseArea.containsMouse ? "#424652" : "#353944"
        border.color: "#1A1C22"
        border.width: 1

        Row {
            anchors.centerIn: parent
            spacing: 6

            Text {
                text: "+"
                color: "#FFFFFF"
                font.pixelSize: 16
                font.weight: Font.Medium
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Add Track"
                color: "#E2E6EF"
                font.pixelSize: 13
                font.weight: Font.Medium
                font.family: "sans-serif"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                console.log("➕ Add Track presionado")
            }
        }
    }
}