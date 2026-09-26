import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "topbar"
import "tcp"
import "timeline"
import "dialogs"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#1E1F24"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ZONA 1: TOP BAR (70px)
        LogicTopBar {
            id: topBar
            Layout.fillWidth: true
            Layout.preferredHeight: 70

            onCollaborationRequested: inviteCollaboratorsDialog.open()
        }

        // ZONA MEDIA: TCP + ARRANGER
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ZONA 2: TCP PANEL (260px)
            LogicTcpPanel {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
            }

            // ZONA 3: TIMELINE / ARRANGER
            LogicTimelinePanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // ZONA 4: BOTTOM BAR
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: "#2D3139"
            border.color: "#1A1C22"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    text: "Sample Rate: 48 kHz / 24-bit"
                    color: "#8F94A0"
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "BPM: 120.00  |  Snap: 1/16"
                    color: "#8F94A0"
                    font.pixelSize: 11
                }
            }
        }
    }

    // ============================================================
    // DIÁLOGO MODAL DE COLABORADORES (Perteneciente al skin Logic)
    // ============================================================
    InviteCollaboratorsDialog {
        id: inviteCollaboratorsDialog
        anchors.fill: parent
        z: 9999

        onInviteSent: (user, message) => {
            console.log(" Invitación enviada a:", user, "| Mensaje:", message)
        }

        onLinkCopied: (link) => {
            console.log(" Enlace copiado al portapapeles:", link)
        }
    }
}