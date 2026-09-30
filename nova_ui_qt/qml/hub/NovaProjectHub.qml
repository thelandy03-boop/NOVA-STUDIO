import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: hubRoot
    anchors.fill: parent
    color: "#0F0F11"

    signal projectOpened()

    // 📂 Selector de carpetas para nuevo proyecto
    FolderDialog {
        id: folderDialog
        title: "Seleccionar carpeta para el proyecto"
        currentFolder: "file://" + StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            var path = folderDialog.selectedFolder.toString()
            if (path.startsWith("file://")) {
                path = path.substring(7) // Quitar prefijo file://
            }
            newProjectPathInput.text = path
        }
    }

    // 📂 Selector de archivos para abrir proyecto existente (.ardour)
    FileDialog {
        id: fileDialog
        title: "Abrir sesión de NOVA STUDIO"
        nameFilters: ["Sesiones de Ardour (*.ardour)"]
        currentFolder: "file://" + StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            var selectedFile = fileDialog.selectedFile.toString()
            if (selectedFile.startsWith("file://")) {
                selectedFile = selectedFile.substring(7)
            }
            if (AudioEngine.openProject(selectedFile)) {
                hubRoot.projectOpened()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 40

        // ── PANEL IZQUIERDO: CONTROLES DE CREACIÓN Y APERTURA ──
        ColumnLayout {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            spacing: 24

            // Título / Logo
            ColumnLayout {
                spacing: 4
                Text {
                    text: "NOVA STUDIO"
                    color: "#FFFFFF"
                    font.pixelSize: 28
                    font.bold: true
                    font.letterSpacing: 1.5
                }
                Text {
                    text: "Motor Híbrido C++ & Ardour Core"
                    color: "#6D7278"
                    font.pixelSize: 11
                }
            }

            // SECCIÓN: NUEVO PROYECTO
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                color: "#16171A"
                border.color: "#2C2D31"
                radius: 6

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        text: "Nuevo Proyecto"
                        color: "#E0E0E0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    // Input Nombre del Proyecto
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text { text: "Nombre:"; color: "#8E8E93"; font.pixelSize: 9 }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 24
                            color: "#1F2024"
                            border.color: nameInput.activeFocus ? "#FFCC00" : "#3E3E42"
                            radius: 3
                            TextInput {
                                id: nameInput
                                anchors.fill: parent
                                anchors.leftMargin: 8; anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Mi Canción"
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                selectByMouse: true
                            }
                        }
                    }

                    // Input Ruta del Proyecto
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text { text: "Ruta del Proyecto:"; color: "#8E8E93"; font.pixelSize: 9 }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Rectangle {
                                Layout.fillWidth: true
                                height: 24
                                color: "#1F2024"
                                border.color: "#3E3E42"
                                radius: 3
                                Text {
                                    id: newProjectPathInput
                                    anchors.fill: parent
                                    anchors.leftMargin: 8; anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/NovaProjects"
                                    color: "#A0A0A5"
                                    font.pixelSize: 10
                                    elide: Text.ElideLeft
                                }
                            }
                            // Botón Examinar
                            Rectangle {
                                width: 28; height: 24
                                radius: 3
                                color: browseMouse.containsMouse ? "#3E3E42" : "#2C2D31"
                                border.color: "#3E3E42"
                                Text { anchors.centerIn: parent; text: "•••"; color: "#FFFFFF"; font.pixelSize: 10 }
                                MouseArea {
                                    id: browseMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: folderDialog.open()
                                }
                            }
                        }
                    }

                    // Botón Crear Proyecto (FL Yellow Style)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        radius: 3
                        color: createMouse.containsMouse ? "#D9B000" : "#FFCC00"

                        Text {
                            anchors.centerIn: parent
                            text: "Crear Proyecto Vacío"
                            color: "#000000"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        MouseArea {
                            id: createMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (AudioEngine.newProject(nameInput.text, newProjectPathInput.text)) {
                                    hubRoot.projectOpened()
                                }
                            }
                        }
                    }
                }
            }

            // SECCIÓN: BOTONES GLOBALES
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                // Botón Abrir desde Disco
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: openDiskMouse.containsMouse ? "#2C2D31" : "#16171A"
                    border.color: "#2C2D31"
                    radius: 4

                    Row {
                        anchors.centerIn: parent
                        spacing: 8
                        Text { text: "📂"; font.pixelSize: 12 }
                        Text { text: "Abrir Proyecto desde Disco..."; color: "#E0E0E0"; font.pixelSize: 11; font.bold: true }
                    }

                    MouseArea {
                        id: openDiskMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fileDialog.open()
                    }
                }
            }

            Item { Layout.fillHeight: true } // Espaciador inferior
        }

        // ── PANEL DERECHO: PROYECTOS RECIENTES ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Text {
                text: "Proyectos Recientes"
                color: "#E0E0E0"
                font.pixelSize: 14
                font.bold: true
            }

            // Listado Dinámico
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#16171A"
                border.color: "#2C2D31"
                radius: 6

                ListView {
                    id: recentListView
                    anchors.fill: parent
                    anchors.margins: 8
                    model: AudioEngine.recentProjects
                    clip: true
                    spacing: 4

                    delegate: Rectangle {
                        width: recentListView.width
                        height: 52
                        radius: 4
                        color: itemMouse.containsMouse ? "#2C2D31" : "transparent"
                        border.color: itemMouse.containsMouse ? "#3E3E42" : "transparent"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: modelData.name || "Proyecto sin Nombre"
                                    color: itemMouse.containsMouse ? "#FFCC00" : "#FFFFFF"
                                    font.pixelSize: 11
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData.lastModified || ""
                                    color: "#6D7278"
                                    font.pixelSize: 9
                                }
                            }

                            Text {
                                text: modelData.path || ""
                                color: "#8E8E93"
                                font.pixelSize: 9
                                elide: Text.ElideLeft
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onDoubleClicked: {
                                var ardourFile = modelData.path + "/" + modelData.name + ".ardour"
                                if (AudioEngine.openProject(ardourFile)) {
                                    hubRoot.projectOpened()
                                }
                            }
                        }
                    }

                    // Mensaje alternativo si no hay proyectos recientes
                    Text {
                        anchors.centerIn: parent
                        text: "No tienes proyectos recientes todavía."
                        color: "#6D7278"
                        font.pixelSize: 11
                        visible: recentListView.count === 0
                    }
                }
            }
        }
    }
}