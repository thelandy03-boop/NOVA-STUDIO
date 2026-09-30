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
            close.accepted = false // Detener el cierre inmediato de la aplicación
            unsavedDialog.closeActionType = "exitApp"
            unsavedDialog.visible = true // Mostrar el diálogo modal de advertencia
        } else {
            // Guardar historial final y liberar recursos de forma limpia
            AudioEngine.closeProject()
        }
    }

    // Navegación principal del DAW (StackView de Alto Rendimiento)
    StackView {
        id: rootStack
        anchors.fill: parent
        
        // 🚀 FL STUDIO STYLE: Arranca DIRECTAMENTE en el editor de música (Workspace)
        initialItem: workspaceComponent

        pushEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        pushExit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 } }
        popEnter:  Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        popExit:   Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 } }
    }

    // ── COMPONENTE: ÁREA DE TRABAJO (WORKSPACE DAW) ──
    Component {
        id: workspaceComponent
        WorkspaceScreen {
            // El botón de volver o cambiar proyecto ahora nos lleva al Hub de proyectos
            onGoBack: {
                if (AudioEngine.isDirty) {
                    unsavedDialog.closeActionType = "showHub"
                    unsavedDialog.visible = true
                } else {
                    rootStack.push(hubComponent)
                }
            }
        }
    }

    // ── COMPONENTE: PANTALLA DE INICIO / PROYECTOS RECIENTES (PROJECT HUB) ──
    Component {
        id: hubComponent
        NovaProjectHub {
            onProjectOpened: {
                // Volver de forma inmediata al editor de música al abrir un proyecto
                rootStack.pop()
            }
        }
    }

    // ── OVERLAY MODAL: DIÁLOGO DE CAMBIOS SIN GUARDAR ──
    NovaUnsavedDialog {
        id: unsavedDialog
        anchors.fill: parent
        visible: false
        
        // Tipo de acción al confirmar: "exitApp" | "showHub"
        property string closeActionType: "exitApp"

        onSaveAndExit: {
            AudioEngine.saveProject()
            executeClosingAction()
        }

        onDiscardAndExit: {
            executeClosingAction()
        }

        onCancelClose: {
            unsavedDialog.visible = false
            closeActionType = "exitApp"
        }

        function executeClosingAction() {
            unsavedDialog.visible = false
            
            if (closeActionType === "exitApp") {
                AudioEngine.closeProject()
                Qt.quit() // Salida segura de la aplicación
            } else if (closeActionType === "showHub") {
                AudioEngine.closeProject()
                rootStack.push(hubComponent) // Ir al selector de proyectos
            }
            closeActionType = "exitApp"
        }
    }

    // ── ATAJOS DE TECLADO RÁPIDOS ──

    // 💾 Guardar: Ctrl + S
    Shortcut {
        sequence: "Ctrl+S"
        context: Qt.ApplicationShortcut
        onActivated: {
            AudioEngine.saveProject()
        }
    }

    // 📂 Abrir Hub de Proyectos Recientes: Ctrl + H
    Shortcut {
        sequence: "Ctrl+H"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (rootStack.currentItem !== hubComponent) {
                if (AudioEngine.isDirty) {
                    unsavedDialog.closeActionType = "showHub"
                    unsavedDialog.visible = true
                } else {
                    rootStack.push(hubComponent)
                }
            }
        }
    }

    // ── SKIN ENGINE EN CALIENTE (F1 - F4) ──
    Shortcut {
        sequence: "F1"
        context: Qt.ApplicationShortcut
        onActivated: {
            Theme.activeSkin = "draft"
            console.log("[SKIN ENGINE] Draft (esqueleto)")
        }
    }
    Shortcut {
        sequence: "F2"
        context: Qt.ApplicationShortcut
        onActivated: {
            Theme.activeSkin = "reaper"
            console.log("[SKIN ENGINE] Reaper")
        }
    }
    Shortcut {
        sequence: "F3"
        context: Qt.ApplicationShortcut
        onActivated: {
            Theme.activeSkin = "bandlab"
            console.log("[SKIN ENGINE] BandLab (rack modular)")
        }
    }
    Shortcut {
        sequence: "F4"
        context: Qt.ApplicationShortcut
        onActivated: {
            Theme.activeSkin = "logic"
            console.log("[SKIN ENGINE] Logic (DAW Studio)")
        }
    }
}