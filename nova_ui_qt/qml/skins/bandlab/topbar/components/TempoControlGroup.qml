import QtQuick
import QtQuick.Layouts
import "../icons"

Rectangle {
    id: root

    property int bpm: 120
    property int numerator: 4
    property int denominator: 4

    signal metronomeClicked()
    signal menuClicked()
    signal tempoClicked()

    implicitWidth: tempoRow.implicitWidth
    implicitHeight: 28
    height: 28
    radius: 14
    color: "#141620"
    border.color: "#222534"
    border.width: 1

    RowLayout {
        id: tempoRow
        anchors.fill: parent
        spacing: 0

        // Celda 1: Icono Metrónomo
        Item {
            implicitWidth: 32
            implicitHeight: 28
            IconMetronome {
                anchors.centerIn: parent
                color: "#A0A5B5"
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.metronomeClicked()
            }
        }

        // Línea divisora 1
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 2: Flecha desplegable (Chevron)
        Item {
            implicitWidth: 24
            implicitHeight: 28
            IconChevronDown {
                anchors.centerIn: parent
                color: "#A0A5B5"
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.menuClicked()
            }
        }

        // Línea divisora 2
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 3: 120 bpm
        Row {
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter

            Text { text: root.bpm.toString(); color: "#FFFFFF"; font.pixelSize: 12; font.bold: true }
            Text { text: "bpm"; color: "#6E7280"; font.pixelSize: 10 }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.tempoClicked()
            }
        }

        // Línea divisora 3
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 4: Compás (4 / 4)
        Row {
            Layout.leftMargin: 14
            Layout.rightMargin: 16
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter

            Text { text: root.numerator.toString(); color: "#FFFFFF"; font.pixelSize: 11; font.bold: true }
            Text { text: "/"; color: "#6E7280"; font.pixelSize: 11 }
            Text { text: root.denominator.toString(); color: "#FFFFFF"; font.pixelSize: 11; font.bold: true }
        }
    }
}
