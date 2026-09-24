import QtQuick

DraftSkeleton {
    id: root
    skinLabel: "Draft Skin"

    onRequestHome: {
        if (root.parent && root.parent.parent && root.parent.parent.goBack)
            root.parent.parent.goBack()
    }

    // Dock inferior vacío (Fase 1 se construye aquí)
    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Text {
            anchors.centerIn: parent
            text: "DOCK VACÍO  ·  Draft Skeleton"
            color: "#2A2A32"
            font.pixelSize: 12
            font.bold: true
        }
    }
}
