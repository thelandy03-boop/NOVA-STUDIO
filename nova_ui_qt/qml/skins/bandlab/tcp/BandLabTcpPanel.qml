import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    color: "#0A0A0B"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Estructura limpia 1:1 con BandLab
        MixEditorHeaders {
            Layout.fillWidth: true
            onAddTrackRequested: console.log("[BandLab] Add Track Requested")
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}