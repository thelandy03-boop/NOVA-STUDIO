import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import "components"
import "icons"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#090A0E"
    border.color: "#161822"
    border.width: 1

    signal homeClicked()
    signal undoClicked()
    signal redoClicked()
    signal saveClicked()
    signal publishClicked()

    property bool canUndo: false
    property bool canRedo: false
    property string projectName: "Nuevo proyecto"
    property string lastSavedText: "Nunca"

    // Modal Flotante de Invitar Colaboradores
    InviteCollaboratorsModal {
        id: inviteModal
    }

    // Sub-menú Desplegable Flotante
    BandLabMenuPopup {
        id: mainMenuPopup
        x: menuBtn.x
        y: menuBtn.y + menuBtn.height + 6
        onInviteCollaboratorsClicked: inviteModal.open()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==========================================
        // FILA 1: HEADER (40px)
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#090A0E"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                // --- IZQUIERDA ---
                Row {
                    spacing: 10
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        id: menuBtn
                        width: 28; height: 28; radius: 6
                        color: menuArea.containsMouse ? "#1A1C28" : "transparent"
                        anchors.verticalCenter: parent.verticalCenter

                        IconMenu { anchors.centerIn: parent; color: menuArea.containsMouse ? "#FFFFFF" : "#A0A5B5" }
                        MouseArea { id: menuArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: mainMenuPopup.open() }
                    }

                    Row {
                        spacing: 5
                        anchors.verticalCenter: parent.verticalCenter

                        Canvas {
                            width: 16; height: 16
                            anchors.verticalCenter: parent.verticalCenter
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.clearRect(0,0,width,height);
                                ctx.fillStyle = "#FF3B30";
                                ctx.beginPath(); ctx.arc(8, 10, 4, 0, Math.PI * 2); ctx.fill();
                                ctx.beginPath(); ctx.moveTo(10, 10); ctx.lineTo(10, 3); ctx.lineTo(14, 2); ctx.lineTo(14, 5); ctx.lineTo(10, 6); ctx.closePath(); ctx.fill();
                            }
                            Component.onCompleted: requestPaint()
                        }

                        Text { text: "BandLab"; color: "#FFFFFF"; font.pixelSize: 14; font.bold: true }
                    }

                    Rectangle {
                        implicitWidth: obtRow.implicitWidth + 20; height: 26; radius: 13; color: "#FFA800"
                        anchors.verticalCenter: parent.verticalCenter
                        Row {
                            id: obtRow; anchors.centerIn: parent; spacing: 5
                            Text { text: "Obtener"; color: "#000000"; font.pixelSize: 11; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                            Shape { width: 12; height: 10; anchors.verticalCenter: parent.verticalCenter; ShapePath { fillColor: "#000000"; strokeWidth: 0; PathSvg { path: "M0 10h12l-1-8-2.5 3L6 0 3.5 5 1 2z" } } }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Text { text: root.projectName; color: "#FFFFFF"; font.pixelSize: 13; font.bold: true; Layout.alignment: Qt.AlignCenter }

                Item { Layout.fillWidth: true }

                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignVCenter

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        Text { text: "Último guardado"; color: "#6E7280"; font.pixelSize: 9; horizontalAlignment: Text.AlignRight; anchors.right: parent.right }
                        Text { text: root.lastSavedText; color: "#FFFFFF"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignRight; anchors.right: parent.right }
                    }

                    SaveButton { onClicked: root.saveClicked() }
                    PublishButton { isDisabled: true; onClicked: root.publishClicked() }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#161822" }

        // ==========================================
        // FILA 2: TRANSPORTE Y CONTROLES (40px)
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#090A0E"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                UndoRedoGroup { canUndo: root.canUndo; canRedo: root.canRedo; onUndoClicked: root.undoClicked(); onRedoClicked: root.redoClicked() }

                Rectangle {
                    width: 32; height: 28; radius: 14; color: "#141620"; border.color: "#222534"; border.width: 1
                    IconKeyboard { anchors.centerIn: parent; color: "#6E7280" }
                }

                TempoControlGroup { bpm: 120; numerator: 4; denominator: 4 }

                Rectangle {
                    implicitWidth: 64; height: 28; radius: 14; color: "#141620"; border.color: "#222534"; border.width: 1
                    Text { anchors.centerIn: parent; text: "Clave"; color: "#A0A5B5"; font.pixelSize: 11 }
                }

                Item { Layout.fillWidth: true }

                TransportControlGroup { timeText: "00:00.0" }

                Item { Layout.fillWidth: true }

                MasterizingControlGroup { label: "Masterizando" }

                MasterVolumeControlGroup { volume: 0.7071 }

                Row {
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter

                    // Botón "Invitar" (También abre el modal de colaboradores)
                    Rectangle {
                        implicitWidth: invRow.implicitWidth + 20; height: 28; radius: 14
                        color: invArea.containsMouse ? "#222534" : "#181A24"
                        border.color: "#282B3C"; border.width: 1
                        Row { id: invRow; anchors.centerIn: parent; Text { text: "Invitar"; color: "#FFFFFF"; font.pixelSize: 11; font.bold: true } }
                        MouseArea { id: invArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: inviteModal.open() }
                    }

                    NotificationBellButton { badgeCount: 3 }
                }
            }
        }
    }
}