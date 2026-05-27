# SPEC-171 — LayoutDesigner + SettingsPanel on MovablePopup

Both files were refactored to inherit from `MovablePopup` instead of declaring
their own `Popup { background; enter; exit; closePolicy }` boilerplate. The
public QML API (`open()`, `close()`, `opened`) is identical because
`MovablePopup` itself is a `Popup` subclass.

## Files modified

- `ui/qml/LayoutDesigner.qml`
- `ui/qml/SettingsPanel.qml`

## CMakeLists.txt

- No new files; `LayoutDesigner.qml` and `SettingsPanel.qml` are already in
  the QML module's `QML_FILES`. No CMake edits required.

## Behavioral parity checklist

- `open()` / `close()` still work from `BottomToolbar.qml`
  (lines 44 and 121 — `layoutDesigner.open()` / `settingsPanel.open()`).
- `opened` property still reflects state — inherited from Popup.
- Esc closes both — MovablePopup sets `closePolicy: Popup.CloseOnEscape`.
- **Behavioral change**: the old SettingsPanel and LayoutDesigner closed on
  click-outside (`CloseOnPressOutsideParent`). MovablePopup intentionally
  drops that to align with Cheatsheet / About / AutoFill. If any user flow
  depended on click-outside dismissal, re-add per-instance with:

      MovablePopup {
          closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
      }

- Existing visual at first paint matches alpha.31: same surface color,
  border, radius; same dimensions (LayoutDesigner 720×720, SettingsPanel
  820×min(720, parent.height-80)). Note: each modal now also shows
  MovablePopup's own 32px drag header at the top — that is by design (it's
  the shared affordance) and matches Cheatsheet/About.
- SettingsPanel's existing top-right "×" close button is preserved (it was
  intentionally there for redundancy with the header — same as in alpha.31).
- LayoutDesigner inner `saveDialog` Popup is preserved (it is a child
  Popup, not the root popup).

## Re-validation steps

- Bottom toolbar ▦ button → LayoutDesigner opens, drag by header works,
  resize grip resizes, Esc closes.
- Bottom toolbar ⚙ button → Settings opens, all five tabs render the same
  content (Geral / Aparência / Voz / AI Providers / Atualizações).
- `appState.tabSplitMode(...)` interaction inside LayoutDesigner.apply()
  remains untouched — only the popup chrome changed.
