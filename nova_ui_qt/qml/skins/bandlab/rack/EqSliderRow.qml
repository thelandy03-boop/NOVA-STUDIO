import QtQuick

Item {
    id: root

    property string label: "BAND"
    property string valueText: "0.0 dB"
    property real value: 0.5

    signal valueEdited(real v)

    width: parent ? parent.width : 160
    height: 28

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        text: root.label
        color: "#8A8A96"
        font.pixelSize: 9
        font.bold: true
    }

    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        text: root.valueText
        color: "#6E6E78"
        font.pixelSize: 9
    }

    Rectangle {
        id: track
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        height: 4
        radius: 2
        color: "#2A2A30"

        Rectangle {
            width: track.width * root.value
            height: parent.height
            radius: 2
            color: "#F5C542"
        }

        Rectangle {
            x: Math.max(0, Math.min(track.width - 12, track.width * root.value - 6))
            anchors.verticalCenter: parent.verticalCenter
            width: 12
            height: 12
            radius: 6
            color: "#F5C542"
            border.color: "#1A1A1A"
            border.width: 1
        }

        MouseArea {
            anchors.fill: parent
            anchors.topMargin: -8
            anchors.bottomMargin: -8
            cursorShape: Qt.PointingHandCursor
            onPositionChanged: (mouse) => {
                if (pressed) {
                    var v = Math.max(0, Math.min(1, mouse.x / width))
                    root.value = v
                    root.valueEdited(v)
                }
            }
            onClicked: (mouse) => {
                var v = Math.max(0, Math.min(1, mouse.x / width))
                root.value = v
                root.valueEdited(v)
            }
        }
    }
}
