import QtQuick
import "../draft"
import "rack"
import "tcp"
import "topbar"

DraftSkeleton {
    id: root
    skinLabel: "BandLab Skin"
    dockHeight: effectsRack.currentDockHeight

    onRequestHome: {
        if (root.parent && root.parent.parent && root.parent.parent.goBack)
            root.parent.parent.goBack()
    }

    // Inyecta la TopBar real de BandLab
    topBarContent: BandLabTopBar {
        onHomeClicked: root.requestHome()
    }

    // Inyecta el TCP real de BandLab
    tcpContent: BandLabTcpPanel {
        anchors.fill: parent
    }

    // Inyecta el Rack de Efectos inferior
    EffectsRack {
        id: effectsRack
        anchors.fill: parent
        trackNames: root.trackNames
        activeTrack: root.activeTrack
    }
}