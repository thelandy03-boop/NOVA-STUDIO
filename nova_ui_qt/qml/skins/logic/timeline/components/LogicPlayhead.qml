import QtQuick

Item {
    id: root

    property real timelineX: 0
    property real contentX: 0
    property real barWidth: 80.0
    property bool isDragging: headMouseArea.pressed

    // 🔒 Posición X blindada usando helpers C++
    x: isDragging ? (dragScreenX - 12) : (Math.max(0, root.timelineX) - root.contentX - 12)
    property real dragScreenX: 0

    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: 24
    z: 30

    // 1. Aguja vertical roja
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 13
        anchors.bottom: parent.bottom
        width: 2
        color: root.isDragging ? "#FFCC00" : "#FF3B30"
        opacity: 0.95
    }

    // 2. Cabezal de la aguja
    Image {
        id: headIcon
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 2
        width: 18
        height: 18
        source: Qt.resolvedUrl("../icons/svg/LogicPlayheadHead.svg")
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
    }

    // 3. Área interactiva con helper C++ pixelToBeat
    MouseArea {
        id: headMouseArea
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: 28
        height: 32
        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor

        onPressed: (mouse) => updatePosition(mouse)
        onPositionChanged: (mouse) => {
            if (pressed) updatePosition(mouse)
        }

        onReleased: {
            if (typeof AudioEngine !== "undefined") {
                var timelinePixelX = Math.max(0, (root.x + 12) + root.contentX)
                var targetBeat = AudioEngine.pixelToBeat(timelinePixelX, root.barWidth)
                AudioEngine.locateBeat(targetBeat)
            }
        }

        function updatePosition(mouse) {
            if (typeof AudioEngine === "undefined") return;

            var parentContainer = root.parent
            if (!parentContainer) return;

            var screenPt = mapToItem(parentContainer, mouse.x, mouse.y)
            var timelinePixelX = Math.max(0, screenPt.x + root.contentX)
            root.dragScreenX = timelinePixelX - root.contentX

            var targetBeat = AudioEngine.pixelToBeat(timelinePixelX, root.barWidth)
            AudioEngine.locateBeat(targetBeat)
        }
    }
}