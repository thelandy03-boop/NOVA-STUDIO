import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string text: "Button"
    property string iconType: ""
    property bool isActive: false
    property bool hasBadge: false
    property string badgeText: "Beta"
    property bool outlined: true

    signal clicked()

    implicitWidth: contentRow.implicitWidth + 22
    implicitHeight: 28
    radius: 14

    color: {
        if (root.isActive)
            return "#2B2E3C"
        if (hit.containsMouse)
            return "#1E202B"
        return root.outlined ? "#181A22" : "transparent"
    }

    border.width: root.isActive ? 1 : (root.outlined ? 1 : 0)
    border.color: {
        if (root.isActive)
            return "#484E62"
        if (hit.containsMouse)
            return "#2C303E"
        return root.outlined ? "#252834" : "transparent"
    }

    Behavior on color { ColorAnimation { duration: 100 } }
    Behavior on border.color { ColorAnimation { duration: 100 } }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 6

        TrackIcon {
            visible: root.iconType !== ""
            iconType: root.iconType
            iconColor: root.isActive ? "#FFFFFF" : "#7D8392"
            width: 16  // Dimensiones 16x16 para icono completo sin recortes
            height: 16
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.text
            color: root.isActive ? "#FFFFFF" : "#8E94A4"
            font.pixelSize: 11
            font.bold: root.isActive
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            visible: root.hasBadge
            width: badgeLabel.implicitWidth + 10
            height: 16
            radius: 4
            color: root.isActive ? "#3D4255" : "#252834"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: badgeLabel
                anchors.centerIn: parent
                text: root.badgeText
                color: root.isActive ? "#FFFFFF" : "#8E94A4"
                font.pixelSize: 9
                font.bold: true
            }
        }
    }

    MouseArea {
        id: hit
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        onPressed: root.scale = 0.97
        onReleased: root.scale = 1.0
        onCanceled: root.scale = 1.0
    }

    Behavior on scale { NumberAnimation { duration: 80 } }
}   