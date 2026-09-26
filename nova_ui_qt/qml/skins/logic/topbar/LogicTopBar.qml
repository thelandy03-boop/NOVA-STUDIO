import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Rectangle {
    id: root
    implicitHeight: 70
    color: "#2C303A"

    // Señal emitida cuando el usuario selecciona "Colaboración" en Tools
    signal collaborationRequested()

    // Brillo sutil superior
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: "#3F4350"
    }

    // ── 1. GRUPO IZQUIERDO (Logo + Herramientas) ──
    RowLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 0

        LogicLogoBlock { Layout.fillHeight: true }
        LogicToolBlock { Layout.fillHeight: true }
        
        LogicToolsButtonBlock { 
            Layout.fillHeight: true 
            onCollaborationRequested: root.collaborationRequested()
        }
    }

    // ── 2. GRUPO CENTRAL (Transporte + LCD Centrados en Pantalla) ──
    RowLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        LogicTransportBlock { Layout.alignment: Qt.AlignVCenter }
        LogicLcdDisplayBlock { Layout.alignment: Qt.AlignVCenter }
    }

    // ── 3. GRUPO DERECHO (Master Fader + Save) ──
    RowLayout {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 0

        LogicMasterControlBlock { Layout.fillHeight: true }
    }

    // Borde inferior sutil
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#181A22"
    } 
}