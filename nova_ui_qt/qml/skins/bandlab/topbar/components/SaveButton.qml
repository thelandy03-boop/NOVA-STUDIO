import QtQuick
import QtQuick.Layouts
import "../icons"

Rectangle {
    id: root

    property bool isDisabled: false
    property string label: "Guardar"

    signal clicked()

    implicitWidth: contentRow.implicitWidth + 24
    implicitHeight: 28
    height: 28
    radius: 14
    color: isDisabled ? "#12131A" : "#181A24"
    border.color: isDisabled ? "#1E202B" : "#282B3C"
    border.width: 1
    opacity: isDisabled ? 0.5 : 1.0

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6

        IconSaveCloud {
            anchors.verticalCenter: parent.verticalCenter
            color: root.isDisabled ? "#6E7280" : "#A0A5B5"
        }

        Text {
            text: root.label
            color: root.isDisabled ? "#6E7280" : "#FFFFFF"
            font.pixelSize: 11
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: !root.isDisabled
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }
}
