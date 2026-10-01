import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

Item {
    id: hubRoot
    width: parent ? parent.width : 1280
    height: parent ? parent.height : 800

    signal projectOpened()

    property bool isBusy: false

    function safeOpenProject(filePath) {
        if (hubRoot.isBusy) return
        hubRoot.isBusy = true

        var cleanPath = filePath.toString()
        if (cleanPath.startsWith("file://")) {
            cleanPath = cleanPath.substring(7)
        }

        console.log("📂 [GUI] Abriendo proyecto independiente:", cleanPath)
        if (AudioEngine.openProject(cleanPath)) {
            hubRoot.projectOpened()
        }

        Qt.callLater(function() { hubRoot.isBusy = false })
    }

    function createInstantProject() {
        if (hubRoot.isBusy) return
        hubRoot.isBusy = true

        var projectsDir = StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/NovaProjects"
        var count = AudioEngine.recentProjects ? AudioEngine.recentProjects.length + 1 : 1
        var projectName = "Proyecto " + count

        console.log("⚡ [GUI] Creando carpeta de proyecto independiente:", projectName)
        if (AudioEngine.newProject(projectName, projectsDir)) {
            hubRoot.projectOpened()
        }

        Qt.callLater(function() { hubRoot.isBusy = false })
    }

    Rectangle {
        anchors.fill: parent
        color: "#0F0F11"
    }

    FileDialog {
        id: fileDialog
        title: "Abrir sesión de NOVA STUDIO"
        nameFilters: ["Sesiones de Ardour (*.ardour)"]
        currentFolder: "file://" + StandardPaths.writableLocation(StandardPaths.DocumentsLocation) + "/NovaProjects"
        onAccepted: {
            hubRoot.safeOpenProject(fileDialog.selectedFile)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 50
        spacing: 50

        // ── PANEL IZQUIERDO: ACCIONES RÁPIDAS ──
        ColumnLayout {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            spacing: 32

            ColumnLayout {
                spacing: 6
                Text {
                    text: "NOVA STUDIO"
                    color: "#FFFFFF"
                    font.pixelSize: 32
                    font.bold: true
                    font.letterSpacing: 2
                }
                Text {
                    text: "Motor Híbrido C++ & Ardour Core"
                    color: "#6D7278"
                    font.pixelSize: 11
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    radius: 6
                    color: createMouse.containsMouse ? "#D9B000" : "#FFCC00"

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "⚡"; font.pixelSize: 16 }
                        Text {
                            text: "Crear Nuevo Proyecto"
                            color: "#000000"
                            font.pixelSize: 13
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: createMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: hubRoot.createInstantProject()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 42
                    color: openDiskMouse.containsMouse ? "#2C2D31" : "#16171A"
                    border.color: openDiskMouse.containsMouse ? "#4E4F56" : "#2C2D31"
                    border.width: 1
                    radius: 6

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 10
                        Text { text: "📂"; font.pixelSize: 14 }
                        Text {
                            text: "Abrir Proyecto desde Disco..."
                            color: "#E0E0E0"
                            font.pixelSize: 12
                            font.bold: true
                        }
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

            Item { Layout.fillHeight: true }
        }

        // ── PANEL DERECHO: PROYECTOS RECIENTES CON BOTÓN DE ELIMINAR ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            Text {
                text: "Proyectos Recientes"
                color: "#FFFFFF"
                font.pixelSize: 15
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#16171A"
                border.color: "#2C2D31"
                radius: 8

                ListView {
                    id: recentListView
                    anchors.fill: parent
                    anchors.margins: 10
                    model: AudioEngine.recentProjects
                    clip: true
                    spacing: 6

                    delegate: Rectangle {
                        width: recentListView.width
                        height: 54
                        radius: 5
                        color: itemMouse.containsMouse ? "#2C2D31" : "#1C1D21"
                        border.color: itemMouse.containsMouse ? "#FFCC00" : "#28292E"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: modelData.name || "Proyecto sin Nombre"
                                        color: itemMouse.containsMouse ? "#FFCC00" : "#FFFFFF"
                                        font.pixelSize: 12
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

                            // 🗑️ BOTÓN DE ELIMINAR DE RECIENTES
                            Rectangle {
                                width: 24; height: 24; radius: 12
                                color: deleteRecentMouse.containsMouse ? "#40FF3B30" : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    color: deleteRecentMouse.containsMouse ? "#FF3B30" : "#6D7278"
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                MouseArea {
                                    id: deleteRecentMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        AudioEngine.removeRecentProject(modelData.path)
                                    }
                                }
                            }
                        }

                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            anchors.rightMargin: 36 // No tapar el botón de borrar
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var ardourFile = modelData.path + "/" + modelData.name + ".ardour"
                                hubRoot.safeOpenProject(ardourFile)
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "No tienes proyectos recientes todavía."
                        color: "#6D7278"
                        font.pixelSize: 12
                        visible: recentListView.count === 0
                    }
                }
            }
        }
    }
}