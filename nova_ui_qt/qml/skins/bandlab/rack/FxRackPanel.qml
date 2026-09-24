import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string trackName: "Track 1"

    color: "#0E0F13"
    border.color: "#1A1C23"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Text {
            text: "Rack de efectos  ·  " + root.trackName
            color: "#C8C8D0"
            font.pixelSize: 13
            font.bold: true
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: effectsRow.implicitWidth + 8
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick

            Row {
                id: effectsRow
                spacing: 12

                EffectCard { title: "ECUALIZADOR"; toggled: true; Column { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.headerBottom; anchors.topMargin: 8; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 10; EqSliderRow { label: "GRAVES"; valueText: "0.0 dB"; value: 0.5 } EqSliderRow { label: "MEDIOS"; valueText: "0.0 dB"; value: 0.5 } EqSliderRow { label: "AGUDOS"; valueText: "0.0 dB"; value: 0.5 } } }
                EffectCard { title: "REVERB"; toggled: true; Column { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.headerBottom; anchors.topMargin: 8; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 12; EqSliderRow { label: "ESPACIO"; valueText: "0%"; value: 0.0 } Row { spacing: 6; Rectangle { width: 64; height: 28; radius: 6; color: "#2A2A30"; Text { anchors.centerIn: parent; text: "Mono"; color: "#A0A0A8"; font.pixelSize: 11; font.bold: true } } Rectangle { width: 72; height: 28; radius: 6; color: "#FFD54F"; Text { anchors.centerIn: parent; text: "Estéreo"; color: "#1A1A1A"; font.pixelSize: 11; font.bold: true } } } } }
                EffectCard { title: "DELAY"; toggled: true; Column { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.headerBottom; anchors.topMargin: 8; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 10; EqSliderRow { label: "TIEMPO"; valueText: "250 ms"; value: 0.45 } EqSliderRow { label: "REPITE"; valueText: "25%"; value: 0.25 } EqSliderRow { label: "MEZCLA"; valueText: "0%"; value: 0.0 } } }
                EffectCard { title: "AUTO-PITCH"; toggled: false; width: 200; Text { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.headerBottom; anchors.topMargin: 16; anchors.leftMargin: 14; anchors.rightMargin: 14; text: "Disponible solo en pistas de voz grabada con micrófono"; color: "#4A4D58"; font.pixelSize: 11; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter } }
            }
        }
    }
}