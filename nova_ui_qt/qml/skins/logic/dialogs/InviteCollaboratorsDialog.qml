import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent
    visible: false
    opacity: visible ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation { duration: 150 }
    }

    signal closed()
    signal inviteSent(string user, string message)
    signal linkCopied(string link)

    property string projectLink: "https://www.bandlab.com/join/nova-project-123"

    function open() {
        root.visible = true;
        userInput.forceActiveFocus();
    }

    function close() {
        root.visible = false;
        root.closed();
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.close()

        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.65
        }
    }

    Rectangle {
        id: modalCard
        width: Math.min(480, parent.width - 32)
        height: mainLayout.implicitHeight + 40
        anchors.centerIn: parent

        radius: 8
        color: "#1F2228"
        border.color: "#2D313B"
        border.width: 1

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            width: 28
            height: 28
            radius: 14
            color: closeMouse.containsMouse ? "#2D313B" : "transparent"
            z: 10

            Text {
                anchors.centerIn: parent
                text: "✕"
                color: "#9A9EAB"
                font.pixelSize: 14
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.close()
            }
        }

        ColumnLayout {
            id: mainLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 20
            spacing: 16

            Text {
                text: "Invitar colaboradores"
                color: "#FFFFFF"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            // INPUT 1: USUARIO / EMAIL
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    radius: 4
                    color: "#14161A"
                    border.color: userInput.activeFocus ? "#00A3FF" : "#2D313B"
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.IBeamCursor
                        onClicked: userInput.forceActiveFocus()
                    }

                    TextInput {
                        id: userInput
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        verticalAlignment: TextInput.AlignVCenter
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        selectByMouse: true
                        activeFocusOnPress: true

                        Text {
                            text: "Nombre de usuario o email"
                            color: "#6B707E"
                            font.pixelSize: 13
                            visible: !userInput.text && !userInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Rectangle {
                    id: sendBtn
                    width: 76
                    height: 38
                    radius: 4
                    enabled: userInput.text.trim().length > 0
                    color: !enabled ? "#2A2D36" : (sendMouse.pressed ? "#0082CC" : (sendMouse.containsMouse ? "#1AB0FF" : "#00A3FF"))

                    Text {
                        anchors.centerIn: parent
                        text: "Enviar"
                        color: sendBtn.enabled ? "#FFFFFF" : "#555A66"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    MouseArea {
                        id: sendMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: sendBtn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            if (sendBtn.enabled) {
                                root.inviteSent(userInput.text, messageInput.text);
                                userInput.text = "";
                                messageInput.text = "";
                                root.close();
                            }
                        }
                    }
                }
            }

            // INPUT 2: MENSAJE (TEXTAREA 100% CLICKEABLE CON SVG)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "Mensaje"
                    color: "#9A9EAB"
                    font.pixelSize: 12
                    font.bold: true
                }

                Rectangle {
                    id: messageBox
                    Layout.fillWidth: true
                    height: 90
                    radius: 4
                    color: "#14161A"
                    border.color: messageInput.activeFocus ? "#00A3FF" : "#2D313B"
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        anchors.rightMargin: 36
                        cursorShape: Qt.IBeamCursor
                        onClicked: messageInput.forceActiveFocus()
                    }

                    TextEdit {
                        id: messageInput
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.rightMargin: 36
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        activeFocusOnPress: true

                        onTextChanged: {
                            if (text.length > 256) {
                                text = text.substring(0, 256);
                            }
                        }

                        Text {
                            text: "Por favor, usa como mínimo 50 caracteres."
                            color: "#6B707E"
                            font.pixelSize: 12
                            visible: !messageInput.text && !messageInput.activeFocus
                        }
                    }

                    // Botón Emoji con SVG renderizado
                    Rectangle {
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 6
                        width: 26
                        height: 26
                        radius: 4
                        color: emojiMouse.containsMouse ? "#2D313B" : "transparent"

                        Image {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            source: Qt.resolvedUrl("../topbar/icons/svg/IconEmoji.svg")
                            opacity: emojiMouse.containsMouse ? 1.0 : 0.7
                        }

                        MouseArea {
                            id: emojiMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                messageInput.forceActiveFocus();
                                messageInput.insert(messageInput.cursorPosition, "🎵");
                            }
                        }
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignRight
                    text: messageInput.text.length + "/256"
                    color: messageInput.text.length >= 256 ? "#FF4D4D" : "#6B707E"
                    font.pixelSize: 11
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2D313B"
            }

            // LINK DE INVITACIÓN
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: "Invitar a otros"
                        color: "#FFFFFF"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text {
                        text: "Cualquiera con este enlace puede unirse y editar el proyecto"
                        color: "#9A9EAB"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }

                Rectangle {
                    id: copyBtn
                    width: 104
                    height: 32
                    radius: 4
                    color: copyMouse.pressed ? "#353A46" : (copyMouse.containsMouse ? "#2D313B" : "#242730")
                    border.color: "#3A3F4D"
                    border.width: 1

                    property bool copied: false

                    Text {
                        anchors.centerIn: parent
                        text: copyBtn.copied ? "¡Copiado!" : "Copiar enlace"
                        color: copyBtn.copied ? "#00FF88" : "#FFFFFF"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    MouseArea {
                        id: copyMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            userInput.text = root.projectLink;
                            userInput.selectAll();
                            userInput.copy();
                            userInput.text = "";
                            
                            copyBtn.copied = true;
                            resetCopyTimer.restart();
                            root.linkCopied(root.projectLink);
                        }
                    }

                    Timer {
                        id: resetCopyTimer
                        interval: 2000
                        onTriggered: copyBtn.copied = false
                    }
                }
            }
        }
    }
}
