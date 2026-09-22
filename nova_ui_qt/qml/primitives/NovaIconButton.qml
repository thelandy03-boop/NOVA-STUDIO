import QtQuick
import "../theme"

Item {
    id: root

    property string variant: "reaper"
    property alias  iconText: iconLabel.text
    property alias  label: subLabel.text
    property bool   toggled: false
    property color  activeColor: Theme.novaIndigo
    property real   btnSize: 22

    signal clicked()

    implicitWidth: btnSize
    implicitHeight: btnSize + (subLabel.visible ? 10 : 0)

    NovaPanel {
        id: bg
        anchors.fill: parent
        variant: root.variant
        panelRadius: Theme.radiusSm
        overrideBg: {
            if (root.toggled) return root.activeColor
            if (mouseArea.pressed) return Theme.bgDarker
            if (mouseArea.containsMouse) return Theme.bgHeader
            return Theme.btnFace
        }
        showBorder: true
    }

    NovaText {
        id: iconLabel
        anchors.centerIn: parent
        anchors.verticalCenterOffset: subLabel.visible ? -3 : 0
        variant: root.variant
        size: "xs"
        font.bold: true
        overrideColor: root.toggled ? "#FFFFFF" : Theme.textLight
    }

    NovaText {
        id: subLabel
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        anchors.horizontalCenter: parent.horizontalCenter
        variant: root.variant
        size: "xs"
        muted: true
        visible: text !== ""
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}