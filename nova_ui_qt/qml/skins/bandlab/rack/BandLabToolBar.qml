import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string activeTab: "editor" // "editor" activo por defecto (igual a tu foto)
    signal tabSelected(string tabId)

    implicitHeight: 46

    RackTexture {
        baseColor: "#0E0F13"
        noiseOpacity: 0.04; highlightOpacity: 0.08
        showTopBevel: true; showBottomShadow: false
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 16

        // 1. GRUPO IZQUIERDO
        ToolBarSegmentedGroup {
            options: [
                { id: "autopitch", text: "AutoPitch™", icon: "autopitch" },
                { id: "fx", text: "Efectos", icon: "fx" },
                { id: "editor", text: "Editor", icon: "edit" }
            ]
            activeId: root.activeTab
            onSelected: (id) => {
                root.activeTab = id;
                root.tabSelected(id);
            }
        }

        Item { Layout.fillWidth: true }

        // 2. GRUPO DERECHO
        Row {
            spacing: 4
            Layout.alignment: Qt.AlignVCenter

            ToolBarPillButton {
                text: "Letras/notas"
                iconType: "feather"
                isActive: root.activeTab === "lyrics"
                onClicked: { root.activeTab = "lyrics"; root.tabSelected("lyrics"); }
            }
            ToolBarPillButton {
                text: "BandLab Sounds"
                iconType: "sounds"
                isActive: root.activeTab === "sounds"
                onClicked: { root.activeTab = "sounds"; root.tabSelected("sounds"); }
            }
            ToolBarPillButton {
                text: "Ritmos"
                iconType: "rhythm"
                isActive: root.activeTab === "rhythms"
                onClicked: { root.activeTab = "rhythms"; root.tabSelected("rhythms"); }
            }
            ToolBarPillButton {
                text: "Asistente"
                iconType: "assistant"
                hasBadge: true
                badgeText: "Beta"
                isActive: root.activeTab === "assistant"
                onClicked: { root.activeTab = "assistant"; root.tabSelected("assistant"); }
            }
        }
    }
}