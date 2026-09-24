import QtQuick
import QtQuick.Layouts

Item {
    id: root
    width: parent ? parent.width : 280
    implicitHeight: col.implicitHeight

    signal addTrackRequested()

    Column {
        id: col
        width: parent.width
        spacing: 0

        // 1. ActionBar superior (+ Añadir pista | Nodos | Stretch)
        MixEditorHeadersActionBar {
            width: parent.width
            onAddTrackClicked: root.addTrackRequested()
        }

        // 2. BlockHeaders (Placeholder | AutoMix)
        MixEditorBlockHeaders {
            width: parent.width
        }
    }
}