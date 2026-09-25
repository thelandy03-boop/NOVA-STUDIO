import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../icons"

Rectangle {
    id: root

    property bool isMetronomeEnabled: false
    property int bpm: 120
    property int numerator: 4
    property int denominator: 4

    signal toggleMetronome()
    signal openMetronomeSettings()
    signal bpmClicked()
    signal timeSignatureClicked()

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

        // Celda 1: Botón Activar Metrónomo
        Item {
            implicitWidth: 32
            implicitHeight: 28

            IconMetronome {
                anchors.centerIn: parent
                color: root.isMetronomeEnabled ? "#FF3B30" : "#A0A5B5"
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.isMetronomeEnabled = !root.isMetronomeEnabled;
                    root.toggleMetronome();
                }
            }
        }

        // Divisor 1
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 2: Botón Dropdown Ajustes Metrónomo (Chevron Down)
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
                onClicked: root.openMetronomeSettings()
            }
        }

        // Divisor 2
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 3: Selector / Input de BPM
        Row {
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.bpm.toString()
                color: "#FFFFFF"
                font.pixelSize: 12
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "bpm"
                color: "#6E7280"
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.bpmClicked()
            }
        }

        // Divisor 3
        Rectangle { width: 1; Layout.fillHeight: true; color: "#222534" }

        // Celda 4: Compás (4 / 4)
        Row {
            Layout.leftMargin: 12
            Layout.rightMargin: 14
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.numerator.toString()
                color: "#FFFFFF"
                font.pixelSize: 11
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "/"
                color: "#6E7280"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.denominator.toString()
                color: "#FFFFFF"
                font.pixelSize: 11
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.timeSignatureClicked()
            }
        }
    }
}
