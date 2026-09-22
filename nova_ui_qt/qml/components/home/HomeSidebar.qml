import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../theme"

Rectangle {
    id: root
    signal openStudioRequested()
    color: Theme.homeSidebar
    property int selectedIndex: 0
    readonly property var items: ["My projects", "My tracks", "Soundshop", "Tutorials"]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        Row {
            spacing: 10
            Layout.bottomMargin: 16

            Rectangle {
                width: 32; height: 32; radius: 8
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.novaIndigo }
                    GradientStop { position: 1.0; color: Theme.novaPurple }
                }
                Text {
                    anchors.centerIn: parent
                    text: "N"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                }
            }
            Text {
                text: "Nova Studio"
                color: Theme.textLight
                font.pixelSize: 16
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Repeater {
            model: root.items
            delegate: Item {
                id: delegateItem
                width: parent.width
                height: 40
                required property int index
                required property string modelData

                Rectangle {
                    anchors.fill: parent
                    radius: 20
                    color: root.selectedIndex === delegateItem.index ? Qt.rgba(1, 1, 1, 0.12) : "transparent"
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    text: delegateItem.modelData
                    color: root.selectedIndex === delegateItem.index ? Theme.textLight : "#94A3B8"
                    font.pixelSize: 13
                    font.bold: root.selectedIndex === delegateItem.index
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.selectedIndex = delegateItem.index
                }
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            radius: 12
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.novaIndigo }
                GradientStop { position: 1.0; color: Theme.novaPurple }
            }
            Column {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    text: "NOVA Premium"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 13
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "Unlock AI tools"
                    color: "#E0E7FF"
                    font.pixelSize: 11
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        Button {
            id: openStudioBtn
            Layout.fillWidth: true
            Layout.topMargin: 8
            text: "Open Studio"
            onClicked: root.openStudioRequested()
            background: Rectangle {
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.1)
                border.color: Qt.rgba(1, 1, 1, 0.2)
            }
            contentItem: Text {
                text: openStudioBtn.text
                color: "white"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
