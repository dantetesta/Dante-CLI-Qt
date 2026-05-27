# SPEC-172 — Picture-in-Picture Calculator window

A new QML file `ui/qml/CalculatorPipWindow.qml` declares a borderless,
always-on-top `Window` that embeds `CalculatorView`. Because the in-tab
`CalculatorView` and the PiP both bind to the same `calculator` context
property, memory / display / history stay in sync — they are two views on
the same C++ controller.

## Files added

- `ui/qml/CalculatorPipWindow.qml`

## CMakeLists.txt

- Add `CalculatorPipWindow.qml` to the QML module's `QML_FILES` list. The
  alias-loop that copies QML files into the runtime resource bundle
  (`qt_add_qml_module(... QML_FILES ... )`) must include the new file. No
  other CMake change.

## CalculatorView.qml (1-line patch)

The Calculator pane is in the don't-touch list, so the PiP launcher needs
a tiny edit to the header. Add the following inside the header `RowLayout`
(at line ~127, next to the existing "Histórico" ToolButton):

```qml
ToolButton {
    text: "⛶ PiP"
    onClicked: {
        const c = Qt.createComponent("CalculatorPipWindow.qml")
        if (c.status === Component.Ready) {
            const w = c.createObject(null)
            if (w) w.closed.connect(function() { /* optional tray sync */ })
        } else if (c.status === Component.Error) {
            console.warn("CalculatorPipWindow:", c.errorString())
        }
    }
    contentItem: Text {
        text: parent.text
        color: Theme.fgMuted
        font.family: Theme.fontSans
        font.pixelSize: 12
    }
    background: Rectangle {
        color: parent.hovered ? Theme.surfaceHigh : "transparent"
        radius: Theme.radiusSm
        border.color: Theme.borderSoft
    }
}
```

This mirrors the existing "Histórico" ToolButton's chrome so the new button
visually belongs in the same row.

## main.cpp / AppState

- No backend changes. The PiP window inherits `calculator` automatically
  because the engine that creates the child Window is the same one that
  registered the context property.
- Optional polish: register a system tray icon (QSystemTrayIcon) with a
  "Mostrar Calculadora" action that re-creates the PiP if the user closes
  it but wants it back without going through the tab.

## Verification

- Click the PiP button in the in-tab CalculatorView header → a small
  borderless window appears, always on top, centered on the primary
  screen.
- Drag it by its header bar; resize via the bottom-right grip.
- Type digits in the PiP → the in-tab view updates the same display
  (and vice versa) — both bind to `calculator.displayText`.
- Esc closes the PiP. So does its × button.
- Closing the PiP returns control to the in-tab view; the next open
  reuses the same C++ controller (memory, history, lastError survive).
