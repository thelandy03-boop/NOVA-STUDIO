import QtQuick
import QtQuick.Window
import QtQuick.Controls
import "theme"
import "screens"

Window {
    id: mainWindow
    width: 1280
    height: 800
    visible: true
    title: "NOVA Studio v9.8 (Qt 6)"
    color: "#121212"

    StackView {
        id: rootStack
        anchors.fill: parent
        initialItem: homeComponent

        pushEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 250 } }
        pushExit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 250 } }
        popEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 250 } }
        popExit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 250 } }
    }

    Component {
        id: homeComponent
        HomeScreen {
            onOpenStudio: rootStack.push(workspaceComponent)
        }
    }

    Component {
        id: workspaceComponent
        WorkspaceScreen {
            onGoBack: rootStack.pop()
        }
    }
}
