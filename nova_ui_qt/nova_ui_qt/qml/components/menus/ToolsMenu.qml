import QtQuick
import QtQuick.Controls

Menu {
    id: toolsMenu
    title: "Tools"

    signal openCollaboratorsRequested()

    background: Rectangle {
        implicitWidth: 180
        color: "#1A1C20"
        border.color: "#2D313B"
        border.width: 1
        radius: 6
    }

    MenuItem {
        text: "👥  Colaboradores"
        
        contentItem: Text {
            text: parent.text
            color: "#FFFFFF"
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: parent.highlighted ? "#00A3FF" : "transparent"
            radius: 4
        }

        onTriggered: {
            toolsMenu.openCollaboratorsRequested()
        }
    }

    MenuSeparator {
        contentItem: Rectangle {
            implicitHeight: 1
            color: "#2D313B"
        }
    }

    MenuItem {
        text: "⚙️  Ajustes de proyecto"
        
        contentItem: Text {
            text: parent.text
            color: "#A0A5B5"
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: parent.highlighted ? "#2D313B" : "transparent"
            radius: 4
        }
    }
}
