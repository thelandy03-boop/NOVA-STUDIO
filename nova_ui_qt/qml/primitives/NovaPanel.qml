import QtQuick
import "../theme"

Rectangle {
    id: root

    property string variant: "reaper"   // "reaper" | "home"
    property real   panelRadius: Theme.radiusMd
    property color  overrideBg: "transparent"
    property bool   showBorder: true

    readonly property color _bg: overrideBg !== "transparent"
        ? overrideBg
        : (variant === "home" ? Theme.homeCard : Theme.bgPanel)

    readonly property color _border: variant === "home"
        ? Theme.homeCardBorder
        : Theme.btnBorder

    radius: panelRadius
    color: _bg
    border.color: showBorder ? _border : "transparent"
    border.width: showBorder ? 1 : 0
    antialiasing: true
}