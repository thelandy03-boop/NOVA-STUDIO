import QtQuick
import QtQuick.Layouts
import "../../theme"

Rectangle {
    id: root
    color: "#1E1E1E"
    property int selectedTrack: 2

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TransportBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1A1A1A"

            Row {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 2

                MasterStrip { height: parent.height }

                Repeater {
                    model: 4
                    TrackStrip {
                        height: parent.height
                        trackNumber: String(index + 1)
                        isSelected: root.selectedTrack === index
                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            onClicked: root.selectedTrack = index
                        }
                    }
                }
            }
        }
    }
}