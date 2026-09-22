import QtQuick
import "../theme"

Text {
    id: root

    property string variant: "reaper"   // "reaper" | "home"
    property string size:    "md"       // "xs" | "sm" | "md" | "lg" | "xl" | "hero"
    property bool   muted:   false
    property color  overrideColor: "transparent"

    readonly property int _px: {
        switch (size) {
            case "xs":   return Theme.fontSizeXs
            case "sm":   return Theme.fontSizeSm
            case "lg":   return Theme.fontSizeLg
            case "xl":   return Theme.fontSizeXl
            case "hero": return Theme.fontSizeHero
            default:     return Theme.fontSizeMd
        }
    }

    readonly property color _col: {
        if (overrideColor !== "transparent") return overrideColor
        if (variant === "home")
            return muted ? Theme.textMuted : Theme.textDark
        return muted ? Theme.textMuted : Theme.textLight
    }

    font.family: Theme.fontFamily
    font.pixelSize: _px
    color: _col
    antialiasing: true
    renderType: Text.NativeRendering
}