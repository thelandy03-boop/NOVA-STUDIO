import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: popup

    padding: 0
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    property string activeCategory: "proyecto"

    signal inviteCollaboratorsClicked()

    background: Rectangle {
        color: "transparent"
    }

    Row {
        spacing: 4

        // PANEL IZQUIERDO
        Rectangle {
            width: 145
            implicitHeight: leftCol.implicitHeight + 16
            color: "#0E1017"
            border.color: "#282B3C"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: leftCol
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2

                MenuCategoryButton { iconText: "🎵"; label: "Proyecto"; hasChevron: true; isActive: popup.activeCategory === "proyecto"; onClicked: popup.activeCategory = "proyecto" }
                MenuCategoryButton { iconText: "✏️"; label: "Editar"; hasChevron: true; isActive: popup.activeCategory === "editar"; onClicked: popup.activeCategory = "editar" }
                MenuCategoryButton { iconText: "🪄"; label: "Herramientas"; hasChevron: true; isActive: popup.activeCategory === "herramientas"; onClicked: popup.activeCategory = "herramientas" }
                MenuCategoryButton { iconText: "👁️"; label: "Ver"; hasChevron: true; isActive: popup.activeCategory === "ver"; onClicked: popup.activeCategory = "ver" }
                MenuCategoryButton { iconText: "⚙️"; label: "Ajustes"; hasChevron: true; isActive: popup.activeCategory === "ajustes"; onClicked: popup.activeCategory = "ajustes" }
                MenuCategoryButton { iconText: "❓"; label: "Ayuda"; hasChevron: true; isActive: popup.activeCategory === "ayuda"; onClicked: popup.activeCategory = "ayuda" }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#282B3C"; Layout.topMargin: 4; Layout.bottomMargin: 4 }

                MenuCategoryButton { iconText: "🚪"; label: "Salir"; hasChevron: false; isActive: false; onClicked: popup.close() }
            }
        }

        // PANEL DERECHO
        Rectangle {
            width: 250
            implicitHeight: rightColProyecto.visible ? rightColProyecto.implicitHeight + 16 : rightColEditar.implicitHeight + 16
            color: "#0E1017"
            border.color: "#282B3C"
            border.width: 1
            radius: 10

            ColumnLayout {
                id: rightColProyecto
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2
                visible: popup.activeCategory === "proyecto"

                SubMenuItemButton { iconText: "➕"; label: "Nuevo proyecto"; onClicked: popup.close() }
                SubMenuItemButton { iconText: "🕒"; label: "Proyectos recientes"; hasChevron: true; onClicked: popup.close() }
                SubMenuItemButton { iconText: "📄"; label: "Cargar proyecto de demostración"; hasChevron: true; onClicked: popup.close() }
                SubMenuItemButton { iconText: "☁️"; label: "Guardar"; shortcut: "Ctrl + S"; isDisabled: true }
                SubMenuItemButton { iconText: "🌐"; label: "Publicar"; isDisabled: true }

                SubMenuItemButton {
                    iconText: "👥"
                    label: "Colaboradores"
                    onClicked: {
                        popup.close();
                        popup.inviteCollaboratorsClicked();
                    }
                }

                SubMenuItemButton { iconText: "⏱️"; label: "Historial de versiones"; isDisabled: true }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#282B3C"; Layout.topMargin: 4; Layout.bottomMargin: 4 }

                SubMenuItemButton { iconText: "📥"; label: "Descargar"; hasChevron: true; onClicked: popup.close() }
            }

            ColumnLayout {
                id: rightColEditar
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2
                visible: popup.activeCategory === "editar"

                SubMenuItemButton { iconText: "↩️"; label: "Deshacer"; shortcut: "Ctrl + Z"; onClicked: popup.close() }
                SubMenuItemButton { iconText: "↪️"; label: "Rehacer"; shortcut: "Ctrl + Y"; onClicked: popup.close() }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                visible: popup.activeCategory !== "proyecto" && popup.activeCategory !== "editar"

                Text { text: "Opciones de " + popup.activeCategory; color: "#A0A5B5"; font.pixelSize: 11; Layout.alignment: Qt.AlignCenter }
            }
        }
    }

    component MenuCategoryButton: Rectangle {
        id: catBtn
        property string iconText: ""
        property string label: ""
        property bool hasChevron: false
        property bool isActive: false
        signal clicked()

        Layout.fillWidth: true; implicitHeight: 32; radius: 6
        color: catBtn.isActive ? "#181A24" : (catArea.containsMouse ? "#141620" : "transparent")

        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 8
            Text { text: catBtn.iconText; font.pixelSize: 12; Layout.alignment: Qt.AlignVCenter }
            Text { text: catBtn.label; color: catBtn.isActive ? "#FFFFFF" : (catArea.containsMouse ? "#E0E4F0" : "#A0A5B5"); font.pixelSize: 12; font.bold: catBtn.isActive; Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter }
            Text { visible: catBtn.hasChevron; text: "›"; color: catBtn.isActive ? "#FFFFFF" : "#6E7280"; font.pixelSize: 14; Layout.alignment: Qt.AlignVCenter }
        }
        MouseArea { id: catArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: catBtn.clicked() }
    }

    component SubMenuItemButton: Rectangle {
        id: subBtn
        property string iconText: ""
        property string label: ""
        property string shortcut: ""
        property bool hasChevron: false
        property bool isDisabled: false
        signal clicked()

        Layout.fillWidth: true; implicitHeight: 32; radius: 6
        color: subBtn.isDisabled ? "transparent" : (subArea.containsMouse ? "#181A24" : "transparent")
        opacity: subBtn.isDisabled ? 0.4 : 1.0

        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 10
            Text { text: subBtn.iconText; font.pixelSize: 12; Layout.alignment: Qt.AlignVCenter }
            Text { text: subBtn.label; color: subBtn.isDisabled ? "#6E7280" : (subArea.containsMouse ? "#FFFFFF" : "#D0D4E0"); font.pixelSize: 12; Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter }
            Text { visible: subBtn.shortcut !== ""; text: subBtn.shortcut; color: "#6E7280"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
            Text { visible: subBtn.hasChevron; text: "›"; color: "#6E7280"; font.pixelSize: 14; Layout.alignment: Qt.AlignVCenter }
        }
        MouseArea { id: subArea; anchors.fill: parent; enabled: !subBtn.isDisabled; hoverEnabled: true; cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor; onClicked: subBtn.clicked() }
    }
}
