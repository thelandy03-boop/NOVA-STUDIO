import QtQuick
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 280

    property real tracksTop: 0
    property real timelineLeft: 0
    property bool showPlaceholder: true

    signal autoMixClicked()

    implicitHeight: col.implicitHeight

    Column {
        id: col
        width: parent.width
        spacing: 0

        // Placeholder Skeleton SVG
        TrackHeaderPlaceholder {
            visible: root.showPlaceholder
            width: parent.width
        }

        // AutoMix Button
        AutoMixButton {
            width: parent.width
            onClicked: root.autoMixClicked()
        }
    }
}