pragma Singleton
import QtQuick

QtObject {
    id: root

    // ──────────────────────────────────────────────
    // 1. HOME — Neo-minimal Amped (Tus tokens originales)
    // ──────────────────────────────────────────────
    readonly property color novaIndigo:      "#6366F1"
    readonly property color novaPurple:      "#A855F7"
    readonly property color homeSidebar:     "#0F172A"
    readonly property color homeMain:        "#F8FAFC"
    readonly property color homeCard:        "#FFFFFF"
    readonly property color homeCardBorder:  "#E2E8F0"
    readonly property color textDark:        "#1E293B"
    readonly property color textMuted:       "#64748B"
    readonly property color textLight:       "#F8FAFC"

    // ──────────────────────────────────────────────
    // 2. WORKSPACE / REAPER (Tus tokens originales)
    // ──────────────────────────────────────────────
    readonly property color bgDarker:        "#121212"
    readonly property color bgDark:          "#1E1E1E"
    readonly property color bgPanel:         "#2A2A2A"
    readonly property color bgHeader:        "#3A3A3A"
    readonly property color bgSelected:      "#3A3A3A"
    readonly property color badgeSelected:   "#D6D6D6"
    readonly property color badgeIdle:       "#2A2A2A"

    readonly property color meterGreen:      "#3DDC84"
    readonly property color meterYellow:     "#F5C542"
    readonly property color meterRed:        "#E5243B"

    readonly property color muteRed:         "#E57373"
    readonly property color soloYellow:      "#FFD54F"
    readonly property color recRed:          "#B71C1C"
    readonly property color faderRail:       "#0A0A0A"
    readonly property color btnFace:         "#2C2C2C"
    readonly property color btnBorder:       "#181818"
    readonly property color btnBorderHi:     "#4A4A4A"

    // ──────────────────────────────────────────────
    // 3. REC BUTTONS — Paleta exacta REAPER
    // ──────────────────────────────────────────────
    // Track REC (Domo HEX)
    readonly property color recTrackIdleFill:      "#633942"
    readonly property color recTrackIdleOuter:     "#B65D62"
    readonly property color recTrackIdleInnerRing: "#AC8B90"
    readonly property color recTrackArmedFill:     "#C9264F"
    readonly property color recTrackArmedOuter:    "#FF6B8B"
    readonly property color recTrackArmedInner:    "#FFFFFF"

    // Transport REC (Shell + LED)
    readonly property color recTransportShell:     "#3A3A3A"
    readonly property color recTransportShellHi:   "#5A5A5A"
    readonly property color recTransportShellPress:"#262626"
    readonly property color recTransportLedOff:    "#5A1E24"
    readonly property color recTransportLedOn:     "#E5243B"

    // ──────────────────────────────────────────────
    // 4. TIPOGRAFÍA
    // ──────────────────────────────────────────────
    readonly property string fontFamily:     "Segoe UI"
    readonly property int fontSizeXs:        9
    readonly property int fontSizeSm:        11
    readonly property int fontSizeMd:        13
    readonly property int fontSizeLg:        16
    readonly property int fontSizeXl:        22
    readonly property int fontSizeHero:      32

    // ──────────────────────────────────────────────
    // 5. ESPACIADO (Grid 4px)
    // ──────────────────────────────────────────────
    readonly property int sp1: 4
    readonly property int sp2: 8
    readonly property int sp3: 12
    readonly property int sp4: 16
    readonly property int sp5: 20
    readonly property int sp6: 24
    readonly property int sp8: 32

    // ──────────────────────────────────────────────
    // 6. RADIOS DE BORDE
    // ──────────────────────────────────────────────
    readonly property real radiusSm:   2
    readonly property real radiusMd:   4
    readonly property real radiusLg:   8
    readonly property real radiusXl:   12
    readonly property real radiusFull: 9999

    // ──────────────────────────────────────────────
    // 7. DIMENSIONES DE LAYOUT ESTABLE
    // ──────────────────────────────────────────────
    readonly property int stripWidth:      72
    readonly property int stripCenterX:    36
    readonly property int faderHeight:     120
    readonly property int transportHeight: 44
}