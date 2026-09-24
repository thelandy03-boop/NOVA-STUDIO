import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    // --- PROPIEDADES PÚBLICAS ---
    property string title: "FX"
    property bool toggled: true

    // Alias de conveniencia para que componentes hijos puedan anclarse al header
    property alias headerBottom: headerItem.bottom

    // Slot principal de contenido (Cualquier control dentro de EffectCard aterriza en 'body')
    default property alias content: body.data

    width: 200
    height: 168
    radius: 12
    color: "#14151B"
    border.color: "#252834"
    border.width: 1

    // Textura táctil integrada con radio de esquina de 12px
    RackTexture {
        baseColor: "#14151B"
        cornerRadius: 12
        noiseOpacity: 0.04
        highlightOpacity: 0.08
        showTopBevel: true
        showBottomShadow: true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==========================================
        // 1. ENCABEZADO DE LA TARJETA (TÍTULO + SWITCH)
        // ==========================================
        Item {
            id: headerItem
            Layout.fillWidth: true
            Layout.preferredHeight: 38

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: root.toggled ? "#E8E8EC" : "#6E7280"
                font.pixelSize: 11
                font.bold: true
                font.letterSpacing: 0.5

                Behavior on color { ColorAnimation { duration: 120 } }
            }

            // TOGGLE SWITCH (Estilo BandLab 1:1)
            Rectangle {
                id: switchBg
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 36
                height: 20
                radius: 10

                // Fondo: Amarillo en activo, gris oscuro en inactivo
                color: root.toggled ? "#FFD54F" : "#2C2F3B"
                border.color: root.toggled ? "#FFE082" : "#3D4252"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 120 } }

                // Bolita del switch: SIEMPRE Blanca
                Rectangle {
                    id: switchKnob
                    width: 16
                    height: 16
                    radius: 8
                    anchors.verticalCenter: parent.verticalCenter
                    x: root.toggled ? parent.width - width - 2 : 2
                    color: "#FFFFFF"

                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.InOutQuad } }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggled = !root.toggled
                }
            }
        }

        // Separador sutil bajo el header
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1E212B"
            opacity: 0.6
        }

        // ==========================================
        // 2. CUERPO DE CONTROLES (SLIDERS / MONO-STEREO)
        // ==========================================
        Item {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 14
            Layout.rightMargin: 14
            Layout.topMargin: 8
            Layout.bottomMargin: 10

            // Atenuación visual y deshabilitación cuando el efecto está apagado
            opacity: root.toggled ? 1.0 : 0.35
            enabled: root.toggled

            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }
}