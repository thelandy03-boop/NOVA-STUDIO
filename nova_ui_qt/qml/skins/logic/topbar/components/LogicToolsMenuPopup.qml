import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: popup

    padding: 0
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    signal collaborationClicked()

    background: Rectangle {
        color: "transparent"
    }

    Column {
        spacing: 0

        // Panel de opciones de Tools
        Rectangle {
            width: 200
            implicitHeight: contentCol.implicitHeight + 16
            color: "#252930"
            border.color: "#3A3E48"
            border.width: 1
            radius: 8

            ColumnLayout {
                id: contentCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                SubMenuItemButton {
                    iconText: "🤝"
                    label: "Colaboración"
                    onClicked: {
                        popup.close()
                        popup.collaborationClicked()
                    }
                }

                SubMenuItemButton {
                    iconText: "⚙️"
                    label: "Preferencias"
                    isDisabled: true
                }

                SubMenuItemButton {
                    iconText: "🔧"
                    label: "Configuración de audio"
                    isDisabled: true
                }
            }
        }
    }

    component SubMenuItemButton: Rectangle {
        id: subBtn
        property string iconText: ""
        property string label: ""
        property bool isDisabled: false
        signal clicked()

        Layout.fillWidth: true
        implicitHeight: 36
        radius: 6
        color: subBtn.isDisabled ? "transparent" : (subArea.containsMouse ? "#32363F" : "transparent")
        opacity: subBtn.isDisabled ? 0.5 : 1.0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            Text {
                text: subBtn.iconText
                font.pixelSize: 14
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: subBtn.label
                color: subBtn.isDisabled ? "#6E7280" : (subArea.containsMouse ? "#FFFFFF" : "#D0D4E0")
                font.pixelSize: 13
                font.family: "sans-serif"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }

        MouseArea {
            id: subArea
            anchors.fill: parent
            enabled: !subBtn.isDisabled
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: subBtn.clicked()
        }
    }
}