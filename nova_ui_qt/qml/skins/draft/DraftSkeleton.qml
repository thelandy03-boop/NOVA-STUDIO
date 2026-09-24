import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../theme"

Rectangle {
    id: root
    color: "#0A0A0B"

    // --- API PÚBLICA DEL ESQUELETO ---
    property int activeTrack: 0
    property var trackNames: ["Track 1", "Track 2", "Track 3", "Track 4"]
    property string skinLabel: "Draft Skin"
    property int dockHeight: 42

    // Slot principal de contenido del dock inferior
    default property alias dockContent: dockHost.data

    // Slots inyectables para TCP e TopBar
    property alias tcpContent: tcpHost.data
    property alias topBarContent: topBarHost.data

    readonly property bool hasCustomTcp: tcpHost.children.length > 0
    readonly property bool hasCustomTopBar: topBarHost.children.length > 0

    signal requestHome()
    signal trackSelected(int index)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==========================================
        // 1. TOP BAR CONTAINER (Alto de 82px si hay TopBar de 2 filas)
        // ==========================================
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: root.hasCustomTopBar ? 82 : 40

            // Slot para TopBar inyectada (BandLab)
            Item {
                id: topBarHost
                anchors.fill: parent
            }

            // TopBar por defecto (Draft / Fallback)
            Rectangle {
                anchors.fill: parent
                visible: !root.hasCustomTopBar
                color: "#111113"
                border.color: "#1C1C1F"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 26
                        radius: 13
                        color: backArea.containsMouse ? "#2A2A2E" : "#1A1A1D"
                        border.color: "#2A2A2E"

                        Text {
                            anchors.centerIn: parent
                            text: "← Home"
                            color: "#B0B0B8"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        MouseArea {
                            id: backArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.requestHome()
                        }
                    }

                    Text {
                        text: "NOVA STUDIO"
                        color: "#F5C542"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text {
                        text: "· " + root.skinLabel
                        color: "#5C5C66"
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 24
                        radius: 12
                        color: "#151518"
                        border.color: "#2A2A2E"
                        Text {
                            anchors.centerIn: parent
                            text: "[F1] Draft  [F2] Reaper  [F3] BandLab"
                            color: "#6E6E78"
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }

        // ==========================================
        // 2. CENTRO: TCP (280px) + TIMELINE
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 1

            // Panel TCP Izquierdo (Ancho oficial BandLab: 280px)
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: "#0E0E10"

                // Host para TCP inyectado (BandLab)
                Item {
                    id: tcpHost
                    anchors.fill: parent
                }

                // Fallback nativo del Draft
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    visible: !root.hasCustomTcp

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        color: "#141416"
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: "PISTAS"
                            color: "#8A8A96"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    Repeater {
                        model: root.trackNames
                        delegate: Rectangle {
                            required property int index
                            required property string modelData

                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            color: root.activeTrack === index ? "#1A1A1F" : (tArea.containsMouse ? "#141418" : "transparent")
                            border.color: root.activeTrack === index ? "#2A2A32" : "transparent"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 10
                                spacing: 8

                                Rectangle {
                                    width: 6; height: 28; radius: 3
                                    color: index === 0 ? "#F5C542" : (index === 1 ? "#5B8DEF" : (index === 2 ? "#3DDC84" : "#E57373"))
                                }

                                Text {
                                    text: modelData
                                    color: root.activeTrack === index ? "#FFFFFF" : "#9A9AA4"
                                    font.pixelSize: 12
                                    font.bold: root.activeTrack === index
                                    Layout.fillWidth: true
                                }
                            }

                            MouseArea {
                                id: tArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    root.activeTrack = index
                                    root.trackSelected(index)
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // Timeline
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0B0B0D"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        color: "#121214"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            spacing: 48
                            Repeater {
                                model: 10
                                Text {
                                    required property int index
                                    text: (index + 1) + ".1"
                                    color: "#3A3A44"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            x: 120; width: 2
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            color: "#F5C542"
                            Rectangle {
                                width: 10; height: 10; radius: 2
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                color: "#F5C542"
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "TIMELINE"
                            color: "#222228"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }
                }
            }
        }

        // ==========================================
        // 3. DOCK INFERIOR DINÁMICO
        // ==========================================
        Rectangle {
            id: dockContainer
            Layout.fillWidth: true
            Layout.preferredHeight: root.dockHeight
            color: "#0E0E10"
            border.color: "#1A1A1E"
            border.width: 1
            clip: true

            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
            }

            Item {
                id: dockHost
                anchors.fill: parent
            }
        }
    }
}