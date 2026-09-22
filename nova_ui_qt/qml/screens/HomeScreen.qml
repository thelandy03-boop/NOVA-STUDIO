import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/home"

Item {
    id: root
    signal openStudio()

    Rectangle { anchors.fill: parent; color: Theme.homeMain }

    RowLayout {
        anchors.fill: parent; spacing: 0

        HomeSidebar {
            Layout.preferredWidth: 240; Layout.fillHeight: true
            onOpenStudioRequested: root.openStudio()
        }

        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 64; color: Theme.homeMain
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 32; anchors.rightMargin: 32
                    Text { text: "My Projects"; color: Theme.textDark; font.pixelSize: 22; font.bold: true; Layout.fillWidth: true }
                    Button {
                        id: topBtn
                        text: "Open Studio"
                        onClicked: root.openStudio()
                        background: Rectangle {
                            radius: 8
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: Theme.novaIndigo }
                                GradientStop { position: 1.0; color: Theme.novaPurple }
                            }
                        }
                        contentItem: Text { text: topBtn.text; color: "white"; font.bold: true; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        leftPadding: 18; rightPadding: 18; topPadding: 10; bottomPadding: 10
                    }
                }
            }

            Row {
                Layout.fillWidth: true; Layout.leftMargin: 32; Layout.rightMargin: 32; Layout.topMargin: 8; spacing: 16
                QuickActionCard { title: "New Project"; subtitle: "Blank session"; accent: Theme.novaIndigo; onClicked: root.openStudio() }
                QuickActionCard { title: "AI Assist"; subtitle: "Generate ideas"; accent: Theme.novaPurple; onClicked: root.openStudio() }
                QuickActionCard { title: "Split Session"; subtitle: "Import stems"; accent: "#0EA5E9"; onClicked: root.openStudio() }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 32; radius: 16
                color: Theme.homeCard; border.color: "#E2E8F0"; border.width: 1
                Column {
                    anchors.centerIn: parent; spacing: 12
                    Text { text: "No projects yet"; color: Theme.textDark; font.pixelSize: 18; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: "Create a new project or open the studio to start."; color: Theme.textMuted; font.pixelSize: 13; anchors.horizontalCenter: parent.horizontalCenter }
                    Button {
                        id: cardBtn
                        anchors.horizontalCenter: parent.horizontalCenter; text: "Open Studio"
                        onClicked: root.openStudio()
                        background: Rectangle { radius: 8; color: Theme.novaIndigo }
                        contentItem: Text { text: cardBtn.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        leftPadding: 16; rightPadding: 16; topPadding: 10; bottomPadding: 10
                    }
                }
            }
        }
    }
}
