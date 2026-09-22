import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/workspace"

Item {
    id: root
    signal goBack()

    Rectangle { anchors.fill: parent; color: "#121212" }

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 36; color: "#1E1E1E"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                Button {
                    id: backBtn
                    text: "← Home"; onClicked: root.goBack(); flat: true
                    contentItem: Text { text: backBtn.text; color: "#AAAAAA"; font.pixelSize: 12 }
                    background: Rectangle { color: "transparent" }
                }
                Text { text: "Nova Studio · Transport / Menus"; color: "#888888"; font.pixelSize: 12; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                Item { width: 64 }
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 1
            Rectangle { Layout.preferredWidth: 180; Layout.fillHeight: true; color: "#1E1E1E"; Text { anchors.centerIn: parent; text: "Track Headers"; color: "#666666"; font.pixelSize: 12 } }
            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#141414"; Text { anchors.centerIn: parent; text: "Arrangement / Clips"; color: "#555555"; font.pixelSize: 14 } }
            Rectangle { Layout.preferredWidth: 220; Layout.fillHeight: true; color: "#1E1E1E"; Text { anchors.centerIn: parent; text: "Sound Library"; color: "#666666"; font.pixelSize: 12 } }
        }

        DeviceRack { Layout.fillWidth: true; Layout.preferredHeight: 310 }
    }
}
