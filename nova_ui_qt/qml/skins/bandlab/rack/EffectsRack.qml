import QtQuick
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    property var trackNames: ["Track 1", "Track 2", "Track 3", "Track 4"]
    property int activeTrack: 0
    property bool showRack: false
    property int currentDockHeight: showRack ? 230 : 42

    signal trackChanged(int index)

    onActiveTrackChanged: root.trackChanged(activeTrack)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. ESPACIO FLEXIBLE
        Item { Layout.fillWidth: true; Layout.fillHeight: true }

        // 2. PANEL DE CARDS DE EFECTOS
        FxRackPanel {
            visible: root.showRack
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            trackName: root.trackNames[Math.min(root.activeTrack, root.trackNames.length - 1)]
        }

        // 3. BARRA DE HERRAMIENTAS
        BandLabToolBar {
            Layout.fillWidth: true
            onTabSelected: (tabId) => {
                if (tabId === "fx") {
                    root.showRack = !root.showRack;
                } else {
                    root.showRack = false;
                }
            }
        }
    }
}