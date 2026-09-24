import QtQuick
import "../theme"
import "../skins"

Item {
    id: root
    signal goBack()

    Rectangle { anchors.fill: parent; color: "#0F1013" }

    // El router decide qué skin mostrar basándose en Theme.activeSkin
    WorkspaceRouter {
        anchors.fill: parent
    }
}
