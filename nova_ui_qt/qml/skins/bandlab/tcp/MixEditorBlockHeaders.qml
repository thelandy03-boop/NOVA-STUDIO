import QtQuick
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 280

    property real tracksTop: 0
    property real timelineLeft: 0
    signal autoMixClicked()

    implicitHeight: col.implicitHeight

    Column {
        id: col
        width: parent.width
        spacing: 0

        // AutoMix Button
        AutoMixButton {
            width: parent.width
            onClicked: root.autoMixClicked()
        }
    }
}
