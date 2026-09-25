import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    implicitWidth: 260
    color: "#21232A"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Cabecera TCP (+ Add Track)
        LogicTcpHeader {
            Layout.fillWidth: true
        }

        // 2. Área para la lista de Tracks (Siguiente paso)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Separador vertical derecho entre TCP y Timeline Arranger
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#14151B"
    }
}