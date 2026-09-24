import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitHeight: 46
    Layout.fillWidth: true

    signal addTrackClicked()
    signal automationClicked()
    signal resizeClicked()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        // Botón "+ Añadir pista"
        Rectangle {
            implicitWidth: addRow.implicitWidth + 24
            implicitHeight: 32
            radius: 16
            color: addHit.containsMouse ? "#222534" : "#181922"
            border.color: addHit.containsMouse ? "#363A4D" : "#262836"
            border.width: 1

            Row {
                id: addRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: "+"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "Añadir pista"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                    font.bold: true
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

        // Botón 1: Automatización (Nodos ☍)
        Rectangle {
            width: 30; height: 30; radius: 15
            color: autoHit.containsMouse ? "#222534" : "#14151D"
            border.color: autoHit.containsMouse ? "#363A4D" : "#222430"
            border.width: 1

            Canvas {
                anchors.centerIn: parent
                width: 14; height: 14
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.strokeStyle = "#8F94A0";
                    ctx.lineWidth = 1.3;
                    ctx.beginPath(); ctx.arc(4, 10, 2, 0, Math.PI * 2); ctx.stroke();
                    ctx.beginPath(); ctx.arc(10, 4, 2, 0, Math.PI * 2); ctx.stroke();
                    ctx.beginPath(); ctx.moveTo(5.5, 8.5); ctx.lineTo(8.5, 5.5); ctx.stroke();
                }
                Component.onCompleted: requestPaint()
            }

            MouseArea {
                id: autoHit
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.automationClicked()
            }
        }

        // Botón 2: Altura de Pistas (Corchetes y Flechas ⤨)
        Rectangle {
            width: 30; height: 30; radius: 15
            color: resHit.containsMouse ? "#222534" : "#14151D"
            border.color: resHit.containsMouse ? "#363A4D" : "#222430"
            border.width: 1

            Canvas {
                anchors.centerIn: parent
                width: 14; height: 14
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.strokeStyle = "#8F94A0";
                    ctx.lineWidth = 1.3;
                    ctx.lineCap = "round";

                    ctx.beginPath(); ctx.arc(3, 7, 5, -Math.PI*0.3, Math.PI*0.3); ctx.stroke();
                    ctx.beginPath(); ctx.arc(11, 7, 5, Math.PI*0.7, Math.PI*1.3); ctx.stroke();
                    ctx.beginPath(); ctx.moveTo(5, 7); ctx.lineTo(9, 7); ctx.stroke();
                }
                Component.onCompleted: requestPaint()
            }

            MouseArea {
                id: resHit
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.resizeClicked()
            }
        }
    }
}