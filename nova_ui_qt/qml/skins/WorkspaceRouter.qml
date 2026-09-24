import QtQuick
import "../theme"

Loader {
    id: skinLoader
    anchors.fill: parent

    source: {
        switch (Theme.activeSkin) {
        case "reaper":
            return Qt.resolvedUrl("reaper/ReaperWorkspace.qml")
        case "bandlab":
            return Qt.resolvedUrl("bandlab/BandLabWorkspace.qml")
        default:
            return Qt.resolvedUrl("draft/DraftWorkspace.qml")
        }
    }
}
