import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property var options: []
    property string activeId: "editor" // "editor" por defecto para ver el efecto activo al iniciar
    signal selected(string optionId)

    implicitHeight: 34
    implicitWidth: layout.implicitWidth + 8
    radius: 17
    color: "#151720"
    border.color: "#222532"
    border.width: 1

    Row {
        id: layout
        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: root.options
            delegate: ToolBarPillButton {
                required property var modelData

                text: modelData.text !== undefined ? modelData.text : ""
                iconType: modelData.icon !== undefined ? modelData.icon : ""
                hasBadge: modelData.hasBadge !== undefined ? modelData.hasBadge : false
                badgeText: modelData.badgeText !== undefined ? modelData.badgeText : "BETA"
                isActive: root.activeId === modelData.id
                outlined: false

                onClicked: {
                    root.activeId = modelData.id; // Actualiza el estado activo
                    root.selected(modelData.id);
                }
            }
        }
    }
}