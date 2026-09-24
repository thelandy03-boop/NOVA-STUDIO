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

    property bool canUndo: false
    property bool canRedo: false
    property string projectName: "Nuevo proyecto"
    property string lastSavedText: "Nunca"

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

                    Text {
                        text: "☰"
                        color: "#A0A5B5"
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
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

                    // Botón "Obtener 👑"
                    Rectangle {
                        implicitWidth: obtRow.implicitWidth + 20
                        height: 26; radius: 13
                        color: "#FFA800"
                        anchors.verticalCenter: parent.verticalCenter

                        Row {
                            id: obtRow
                            anchors.centerIn: parent
                            spacing: 5
                            Text { text: "Obtener"; color: "#000000"; font.pixelSize: 11; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                            
                            Shape {
                                width: 12; height: 10
                                anchors.verticalCenter: parent.verticalCenter
                                ShapePath {
                                    fillColor: "#000000"
                                    strokeWidth: 0
                                    PathSvg { path: "M0 10h12l-1-8-2.5 3L6 0 3.5 5 1 2z" }
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // --- CENTRO: Título ---
                Text {
                    text: root.projectName
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                    Layout.alignment: Qt.AlignCenter
                }

                Item { Layout.fillWidth: true }

                // --- DERECHA ---
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignVCenter

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        Text { text: "Último guardado"; color: "#6E7280"; font.pixelSize: 9; horizontalAlignment: Text.AlignRight; anchors.right: parent.right }
                        Text { text: root.lastSavedText; color: "#FFFFFF"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignRight; anchors.right: parent.right }
                    }

                    TopPillButton { text: "Guardar"; iconSymbol: "☁" }
                    TopPillButton { text: "Publicar"; iconSymbol: "🌐"; isDisabled: true }
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

                // 1. CÁPSULA DESHACER / REHACER
                UndoRedoGroup {
                    canUndo: root.canUndo
                    canRedo: root.canRedo
                    onUndoClicked: root.undoClicked()
                    onRedoClicked: root.redoClicked()
                }

                // 2. Teclado Virtual 🎹
                Rectangle {
                    width: 32; height: 28; radius: 14
                    color: "#141620"
                    border.color: "#222534"
                    border.width: 1
                    
                    IconKeyboard {
                        anchors.centerIn: parent
                        color: "#6E7280"
                    }
                }

                // 3. Tempo & Compás (Invocación Modular)
                TempoControlGroup {
                    bpm: 120
                    numerator: 4
                    denominator: 4
                }

                // 4. Clave
                Rectangle {
                    implicitWidth: 64; height: 28; radius: 14
                    color: "#141620"
                    border.color: "#222534"
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "Clave"; color: "#A0A5B5"; font.pixelSize: 11 }
                }

                Item { Layout.fillWidth: true }

                // 5. Transporte (Invocación Modular)
                TransportControlGroup {
                    timeText: "00:00.0"
                }

                Item { Layout.fillWidth: true }

                // 6. Masterizando (Invocación Modular)
                MasterizingControlGroup {
                    label: "Masterizando"
                }

                // 7. Volumen Maestro (Invocación Modular)
                MasterVolumeControlGroup {
                    volume: 0.7071
                }

                // 8. Invitar & Campana 🔔³
                Row {
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        implicitWidth: invRow.implicitWidth + 20; height: 28; radius: 14
                        color: "#181A24"; border.color: "#282B3C"; border.width: 1
                        Row { id: invRow; anchors.centerIn: parent; Text { text: "Invitar"; color: "#FFFFFF"; font.pixelSize: 11; font.bold: true } }
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 14
                        color: "#181A24"; border.color: "#282B3C"; border.width: 1

                        IconBell {
                            anchors.centerIn: parent
                            color: "#FFFFFF"
                        }

                        Rectangle {
                            width: 13; height: 13; radius: 6.5
                            color: "#E53935"
                            x: 15; y: -2
                            Text { anchors.centerIn: parent; text: "3"; color: "#FFFFFF"; font.pixelSize: 8; font.bold: true }
                        }
                    }
                }
            }
        }
    }

    component TopPillButton: Rectangle {
        property string text: ""
        property string iconSymbol: ""
        property bool isDisabled: false

        implicitWidth: pRow.implicitWidth + 20
        height: 28
        radius: 14
        color: isDisabled ? "#12131A" : "#181A24"
        border.color: isDisabled ? "#1E202B" : "#282B3C"
        border.width: 1
        opacity: isDisabled ? 0.5 : 1.0

        Row {
            id: pRow
            anchors.centerIn: parent
            spacing: 6
            Text { text: iconSymbol; color: isDisabled ? "#8F94A0" : "#FFFFFF"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            Text { text: parent.parent.text; color: isDisabled ? "#8F94A0" : "#FFFFFF"; font.pixelSize: 11; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
        }
    }
}