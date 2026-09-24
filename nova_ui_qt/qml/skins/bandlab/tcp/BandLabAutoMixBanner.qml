import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    implicitHeight: 38
    Layout.fillWidth: true

    color: hit.containsMouse ? "#181A26" : "#12131A"
    border.color: "#1C1E2A"
    border.width: 1

    signal clicked()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 8

        // Icono Fader/AutoMix
        Canvas {
            width: 16; height: 16
            Layout.alignment: Qt.AlignVCenter
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = "#8F94A0";
                ctx.lineWidth = 1.3; ctx.lineCap = "round";
                ctx.beginPath(); ctx.moveTo(2, 8); ctx.lineTo(14, 8); ctx.stroke();
                ctx.beginPath(); ctx.arc(6, 8, 2.2, 0, Math.PI * 2); ctx.stroke();
            }
            Component.onCompleted: requestPaint()
        }

        // Texto AutoMix
        Text {
            text: "AutoMix"
            color: "#FFFFFF"
            font.pixelSize: 12
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

        // Badge IA Azul
        Rectangle {
            width: 20; height: 15; radius: 4
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

        // CORONA DORADA VECTORIAL (Garantizada en Linux/Windows/Mac)
        Canvas {
            width: 14; height: 11
            Layout.alignment: Qt.AlignVCenter
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.fillStyle = "#FFB000";
                ctx.beginPath();
                ctx.moveTo(1, 10);
                ctx.lineTo(13, 10);
                ctx.lineTo(12, 3);
                ctx.lineTo(9.5, 7);
                ctx.lineTo(7, 1);
                ctx.lineTo(4.5, 7);
                ctx.lineTo(2, 3);
                ctx.closePath();
                ctx.fill();
            }
            Component.onCompleted: requestPaint()
        }

        Item { Layout.fillWidth: true }

        // Chevron >
        Text {
            text: "›"
            color: "#6C7080"
            font.pixelSize: 18
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
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