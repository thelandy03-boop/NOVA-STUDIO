import QtQuick
import QtQuick.Window
import QtQuick.Controls
import "theme"
import "screens"
import "hub"
import "dialogs"

Window {
    id: mainWindow
    width: 1280
    height: 800
    visible: true
    title: "NOVA Studio v9.8 (Qt 6) - " + AudioEngine.currentProjectName + (AudioEngine.isDirty ? " *" : "")
    color: "#121212"

    // ── INTERCEPTAR EVENTO DE CIERRE DE LA VENTANA ──
    onClosing: (close) => {
        if (AudioEngine.isDirty) {
            close.accepted = false // Detener el cierre inmediato
            unsavedDialog.closeActionType = "exitApp"
            unsavedDialog.visible = true // Mostrar el diálogo modal
        } else {
            AudioEngine.closeProject()
        }
    }

    // Navegación principal del DAW (StackView de Alto Rendimiento)
    StackView {
        id: rootStack
        anchors.fill: parent
        
        initialItem: (AudioEngine.recentProjects && AudioEngine.recentProjects.length > 0) ? hubComponent : workspaceComponent

        pushEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        pushExit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 } }
        popEnter:  Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        popExit:   Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 } }
    }

    // ── COMPONENTE: ÁREA DE TRABAJO (WORKSPACE DAW) ──
    Component {
        id: workspaceComponent
        WorkspaceScreen {
            onGoBack: {
                if (AudioEngine.isDirty) {
                    unsavedDialog.closeActionType = "showHub"
                    unsavedDialog.visible = true
                } else {
                    AudioEngine.closeProject()
                    if (rootStack.depth > 1) {
                        rootStack.pop()
                    } else {
                        rootStack.push(hubComponent)
                    }
                }
            }
        }
    }

    // ── COMPONENTE: PANTALLA DE INICIO / PROYECTOS RECIENTES (PROJECT HUB) ──
    Component {
        id: hubComponent
        NovaProjectHub {
            onProjectOpened: {
                if (rootStack.depth > 1) {
                    rootStack.pop()
                } else {
                    rootStack.push(workspaceComponent)
                }
            }
        }
    }

    // ── OVERLAY MODAL: DIÁLOGO DE CAMBIOS SIN GUARDAR ──
    NovaUnsavedDialog {
        id: unsavedDialog
        anchors.fill: parent
        visible: false
        
        property string closeActionType: "exitApp"

        onSaveAndExit: {
            AudioEngine.saveProject()
            AudioEngine.closeProject()
            executeClosingAction()
        }

        onDiscardAndExit: {
            // 🗑️ Descartar cambios y eliminar la carpeta física si nunca fue guardado
            AudioEngine.discardProject()
            executeClosingAction()
        }

        onCancelClose: {
            unsavedDialog.visible = false
            closeActionType = "exitApp"
        }

        function executeClosingAction() {
            unsavedDialog.visible = false
            
            if (closeActionType === "exitApp") {
                Qt.quit() // Salida segura
            } else if (closeActionType === "showHub") {
                if (rootStack.depth > 1) {
                    rootStack.pop()
                } else {
                    rootStack.push(hubComponent)
                }
            }
        }
    }

    // 📂 Alternar Hub de Proyectos Recientes: Ctrl + H
    Shortcut {
        sequence: "Ctrl+H"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (rootStack.currentItem !== hubComponent) {
                if (AudioEngine.isDirty) {
                    unsavedDialog.closeActionType = "showHub"
                    unsavedDialog.visible = true
                } else {
                    AudioEngine.closeProject()
                    if (rootStack.depth > 1 && rootStack.currentItem === workspaceComponent) {
                        rootStack.push(hubComponent)
                    } else {
                        rootStack.push(hubComponent)
                    }
                }
            }
        }
    }

    // 💾 Guardado Rápido de Proyecto: Ctrl + S
    Shortcut {
        sequence: "StandardKey.Save"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (AudioEngine.saveProject()) {
                console.log("💾 [Shortcut] Proyecto guardado exitosamente.")
            }
        }
    }

    // ✂️ Cortar Clip en la Posición del Cabezal (Playhead): Tecla S
    Shortcut {
        sequence: "S"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (AudioEngine.splitAtPlayhead()) {
                console.log("✂️ [Shortcut] Clip dividido en el Playhead.")
            }
        }
    }

    // 🗑️ Borrado de Clip Seleccionado con Teclado: Delete o Backspace
    Shortcut {
        sequences: ["Delete", "Backspace"]
        context: Qt.ApplicationShortcut
        onActivated: {
            var selectedIdx = AudioEngine.regions.selectedRegionIndex;
            if (selectedIdx >= 0) {
                console.log("🗑️ [Shortcut] Eliminando clip seleccionado en índice: " + selectedIdx);
                AudioEngine.regions.removeRegion(selectedIdx);
            }
        }
    }
}