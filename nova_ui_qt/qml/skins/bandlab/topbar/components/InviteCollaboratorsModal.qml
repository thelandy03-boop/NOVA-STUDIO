import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Popup {
    id: modal

    width: 508
    implicitHeight: mainLayout.implicitHeight + 88
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    background: Rectangle {
        color: "#131417"
        border.color: "#282A34"
        border.width: 1
        radius: 12

        layer.enabled: true
    }

    // Botón Cerrar (X) arriba a la derecha
    Item {
        width: 32; height: 32
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        z: 10

        Shape {
            anchors.centerIn: parent
            width: 24; height: 24
            scale: 16 / 24
            transformOrigin: Item.Center
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: closeArea.containsMouse ? "#FFFFFF" : "#8F94A0"
                strokeWidth: 0
                PathSvg {
                    path: "m12 13.41 6.8 6.8 1.4-1.42L13.42 12l6.8-6.8-1.42-1.4-6.8 6.78-6.8-6.8L3.8 5.2l6.78 6.8-6.8 6.8 1.42 1.4z"
                }
            }
        }

        MouseArea {
            id: closeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: modal.close()
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 36
        spacing: 20

        // --- CABECERA: Título Centrado ---
        Text {
            text: "Invitar colaboradores"
            color: "#FFFFFF"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 4
            Layout.bottomMargin: 4
        }

        // --- SECCIÓN 1: Input Usuario/Email + Botón Enviar ---
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 28
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 8
                color: "#1B1C22"
                border.color: userInput.activeFocus ? "#484E62" : "#282A34"
                border.width: 1

                TextInput {
                    id: userInput
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    verticalAlignment: Text.AlignVCenter
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    clip: true

                    Text {
                        text: "Nombre de usuario o email"
                        color: "#626573"
                        font.pixelSize: 13
                        visible: !parent.text && !parent.activeFocus
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Botón Enviar
            Rectangle {
                implicitWidth: 92
                height: 40
                radius: 21
                color: userInput.text.length > 0 ? "#FFA800" : "#21222A"
                border.color: userInput.text.length > 0 ? "transparent" : "#282A34"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "Enviar"
                    color: userInput.text.length > 0 ? "#000000" : "#515460"
                    font.pixelSize: 13
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: userInput.text.length > 0
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        userInput.text = "";
                        msgInput.text = "";
                        modal.close();
                    }
                }
            }
        }

        // --- SECCIÓN 2: Mensaje ---
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 7
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "Mensaje"
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: msgInput.text.length + "/256"
                    color: "#626573"
                    font.pixelSize: 12
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 116
                radius: 8
                color: "#1B1C22"
                border.color: msgInput.activeFocus ? "#484E62" : "#282A34"
                border.width: 1

                TextEdit {
                    id: msgInput
                    anchors.fill: parent
                    anchors.margins: 14
                    anchors.bottomMargin: 32
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    wrapMode: TextEdit.Wrap
                    clip: true

                    Text {
                        text: "Por favor, usa como mínimo 50 caracteres."
                        color: "#525562"
                        font.pixelSize: 13
                        visible: !parent.text && !parent.activeFocus
                    }
                }

                // Emoji Picker SVG
                Item {
                    width: 22; height: 22
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10

                    Shape {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        scale: 16 / 24
                        transformOrigin: Item.Center
                        preferredRendererType: Shape.CurveRenderer

                        ShapePath {
                            fillColor: emojiArea.containsMouse ? "#FFFFFF" : "#757885"
                            strokeWidth: 0
                            fillRule: ShapePath.OddEvenFill

                            PathSvg {
                                path: "M12 3a9 9 0 1 0 0 18 9 9 0 0 0 0-18M1 12a11 11 0 1 1 22 0 11 11 0 0 1-22 0 M10 10.5a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m7 0a1.5 1.5 0 1 1-3 0 1.5 1.5 0 0 1 3 0m-5 4.65c1.13 0 2.16-.45 2.9-1.19l1.2 1.22a5.83 5.83 0 0 1-8.2 0l1.2-1.22a4 4 0 0 0 2.9 1.19"
                            }
                        }
                    }

                    MouseArea {
                        id: emojiArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: msgInput.text += " 🙂"
                    }
                }
            }
        }

        // --- DIVISOR LÍNEA (<hr>) ---
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#22242D"
            Layout.topMargin: 30
            Layout.bottomMargin: 6
        }

        // --- SECCIÓN 3: Invitar a otros + Copiar Enlace ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Column {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "Invitar a otros"
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                }

                Text {
                    text: "Cualquiera con este enlace puede unirse\ny editar el proyecto"
                    color: "#757885"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            // Botón Copiar Enlace
            Rectangle {
                id: copyBtn
                property bool copied: false

                implicitWidth: 120
                height: 36
                radius: 18
                color: copyArea.containsMouse ? "#3A3C4A" : "#282A34"

                Text {
                    anchors.centerIn: parent
                    text: copyBtn.copied ? "¡Copiado!" : "Copiar enlace"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: copyArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        copyBtn.copied = true;
                        copyTimer.start();
                    }
                }

                Timer {
                    id: copyTimer
                    interval: 2000
                    onTriggered: copyBtn.copied = false
                }
            }
        }
    }
}
