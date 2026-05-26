// Dante CLI design tokens — VS Code Dark+ / Outlook palette.
// Singleton-style import: `import "qml/Theme.qml" as Theme` or just expose
// a QtObject root that other files reference as a property.
//
// Extended to expose the *terminal* color scheme reactively. The `scheme`
// property is rebound whenever `appState.terminalScheme` changes, which in
// turn drives `termBg`, `termFg`, `termCursor`, `termSelectionBg`, and the
// `termAnsi` array of 16 colors. TerminalView and any QML reading these
// values pick up the change automatically.
pragma Singleton
import QtQuick 6.5

QtObject {
    id: theme

    // Surfaces
    readonly property color ink:           "#0E0E12"
    readonly property color surface:       "#18181C"
    readonly property color surfaceLow:    "#121216"
    readonly property color surfaceHigh:   "#202026"
    readonly property color surfaceTop:    "#2A2A32"

    // Borders
    readonly property color border:        "#2E2E38"
    readonly property color borderSoft:    "#202028"
    readonly property color borderStrong:  "#3C3C48"

    // Text
    readonly property color fg:            "#E6E6EB"
    readonly property color fgStrong:      "#FFFFFF"
    readonly property color fgMuted:       "#9B9BAA"
    readonly property color fgFaint:       "#696978"
    readonly property color fgDim:         "#464652"

    // Accent + semantics
    readonly property color accent:        "#7C82FF"
    readonly property color accentSoft:    "#484C90"
    readonly property color accentDim:     "#262850"
    readonly property color success:       "#2ECC71"
    readonly property color warn:          "#F59E0B"
    readonly property color danger:        "#DC2626"

    // Typography. Qt's font.family takes a single family name (not a CSS
    // fallback list), so pick the most appropriate per platform. The mono
    // chain still uses a fallback string — fonts.font.family accepts that.
    readonly property string fontSans:  Qt.platform.os === "osx"
        ? "Helvetica Neue"
        : "Segoe UI Variable Display"
    readonly property string fontMono:  Qt.platform.os === "osx"
        ? "Menlo"
        : "Cascadia Code"

    // Spacing / radius
    readonly property int radiusSm:  6
    readonly property int radiusMd:  8
    readonly property int radiusLg:  10

    // Motion (single source of truth for tween durations / easings).
    readonly property int  motionFast:    140
    readonly property int  motionStd:     220
    readonly property int  motionSlow:    320
    readonly property int  motionEasing:  Easing.OutCubic

    /* ───────────────────────────────────────────────────────────────────
       Terminal color scheme — bound reactively to appState.terminalScheme.
       Falls back to the registry's default if the stored id is unknown,
       and to a hardcoded TokyoNight-ish palette if `schemes` itself is
       missing (lets QML files preview in isolation, e.g. qmlscene).
       ─────────────────────────────────────────────────────────────────── */
    readonly property string terminalSchemeId:
        (typeof appState !== "undefined" && appState && appState.terminalScheme)
            ? appState.terminalScheme : "TokyoNight"

    readonly property var scheme: (typeof schemes !== "undefined" && schemes)
        ? schemes.findById(theme.terminalSchemeId)
        : null

    readonly property color termBg:          scheme && scheme.bg          ? scheme.bg          : "#1a1b26"
    readonly property color termFg:          scheme && scheme.fg          ? scheme.fg          : "#c0caf5"
    readonly property color termCursor:      scheme && scheme.cursor      ? scheme.cursor      : "#7aa2f7"
    readonly property color termSelectionBg: scheme && scheme.selectionBg ? scheme.selectionBg : "#283457"

    // 16 ANSI colors — index 0..15 matches the standard ordering.
    readonly property var termAnsi: scheme && scheme.ansi && scheme.ansi.length === 16
        ? scheme.ansi
        : ["#15161e","#f7768e","#9ece6a","#e0af68",
           "#7aa2f7","#bb9af7","#7dcfff","#a9b1d6",
           "#414868","#f7768e","#9ece6a","#e0af68",
           "#7aa2f7","#bb9af7","#7dcfff","#c0caf5"]
}
