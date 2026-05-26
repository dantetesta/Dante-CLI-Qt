// Dante CLI design tokens — VS Code Dark+ / Outlook palette.
// Singleton-style import: `import "qml/Theme.qml" as Theme` or just expose
// a QtObject root that other files reference as a property.
pragma Singleton
import QtQuick 6.5

QtObject {
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

    // Typography
    readonly property string fontSans:  "Segoe UI Variable, Segoe UI, system-ui"
    readonly property string fontMono:  "JetBrains Mono, Cascadia Code, Consolas, monospace"

    // Spacing / radius
    readonly property int radiusSm:  6
    readonly property int radiusMd:  8
    readonly property int radiusLg:  10
}
