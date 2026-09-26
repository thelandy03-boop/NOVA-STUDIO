import QtQuick
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 54
    implicitHeight: 70

    signal collaborationRequested()

    // Variable para evitar que el clic de cierre vuelva a abrir el popup
    property bool blockReopen: false

    // Menú flotante desplegable
    LogicToolsMenuPopup {
        id: toolsPopup
        y: root.height + 2
        x: (root.width - width) / 2

        onCollaborationClicked: {
            root.collaborationRequested()
        }

        // Se ejecuta justo cuando el popup se va a cerrar por clic externo
        onAboutToHide: {
            if (toolsMouse.containsMouse) {
                root.blockReopen = true
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 4

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Tools"
            color: toolsMouse.containsMouse ? "#FFFFFF" : "#D0D3D9"
            font.pixelSize: 11
            font.weight: Font.Medium
            font.family: "sans-serif"
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 38
            height: 34
            radius: 5
            color: toolsMouse.pressed ? "#252830" : (toolsMouse.containsMouse ? "#3A3E48" : "#32363F")
            border.color: "#1A1C22"
            border.width: 1

            Image {
                anchors.centerIn: parent
                source: Qt.resolvedUrl("../icons/svg/LogicIconWrench.svg")
                width: 20
                height: 20
                opacity: toolsMouse.containsMouse ? 1.0 : 0.85
            }
        }
    }

    // Interacción al hacer clic en el bloque de Tools
    MouseArea {
        id: toolsMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            // Si el popup se acaba de cerrar porque hicimos clic sobre el propio botón Tools, ignoramos la apertura
            if (root.blockReopen) {
                root.blockReopen = false
                return
            }

            if (toolsPopup.opened) {
                toolsPopup.close()
            } else {
                toolsPopup.open()
            }
        }
    }
}