import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // 🔒 CONTRATO OBLIGATORIO:
    // Todo botón heredado DEBE mantener un implicitWidth e implicitHeight > 0
    implicitWidth: 22
    implicitHeight: 22
    
    width: implicitWidth
    height: implicitHeight

    // Alineación garantizada en Layouts
    Layout.alignment: Qt.AlignVCenter

    // Propiedades y señales contractuales
    property bool active: false
    signal toggled()

    // Respuesta visual global al presionar
    opacity: mouseArea.pressed ? 0.75 : 1.0

    // 🚨 VALIDACIÓN OBLIGATORIA DE ARQUITECTURA
    Component.onCompleted: {
        if (root.implicitWidth <= 0 || root.implicitHeight <= 0) {
            console.error("❌ [NOVA-STUDIO ERROR] El componente '" + root + "' viola la regla de botones: DEBE definir implicitWidth e implicitHeight.")
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: {
            root.active = !root.active
            root.toggled()
        }
    }
}