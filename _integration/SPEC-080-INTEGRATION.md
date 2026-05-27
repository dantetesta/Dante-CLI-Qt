# SPEC-080 — AutoFill engine integration

## CMakeLists.txt

### `core/CMakeLists.txt`
Append to the `add_library(dante-core STATIC ...)` source list:

```
autofill/AutoFillEngine.cpp
autofill/AutoFillEngine.h
```

No other changes — `target_include_directories(dante-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})` already lets consumers `#include "autofill/AutoFillEngine.h"`.

### `apps/desktop-qt/CMakeLists.txt`
Append to the `qt_add_executable(dante-cli WIN32 ...)` source list:

```
src/AutoFillController.cpp
src/AutoFillController.h
```

Register the new QML file (mirrors the pattern used for AboutView/CheatsheetView):

1. In the `foreach(qml_file Main TabBar ...)` block, append `AutoFillDialog` to the list of names.
2. In the `qt_add_qml_module(dante-cli ... QML_FILES ...)` list, append `../../ui/qml/AutoFillDialog.qml`.

No QRC, no Q_QML_SINGLETON_TYPE — it's a regular MovablePopup subclass.

## `apps/desktop-qt/src/PaletteController.cpp`

Two call sites currently emit `terminalWriteRequested(tabId, command)` after the user activates a snippet/favorite (lines 126 and 151 today). For each, swap the direct emit for a new high-level signal `autoFillRequested(text)` whenever the text contains placeholders:

```cpp
// Snippet/favorite callback — replace:
//   emit terminalWriteRequested(tabId, command);
// with:
if (dante::autofill::AutoFillEngine::scan(command).isEmpty()) {
    emit terminalWriteRequested(tabId, command);
} else {
    emit autoFillRequested(command);
}
```

Add the matching declaration to `PaletteController.h`:

```cpp
signals:
    void autoFillRequested(const QString& text);
```

Include `"autofill/AutoFillEngine.h"` at the top of `PaletteController.cpp`.

## `apps/desktop-qt/src/main.cpp`

Instantiate the controller, expose it to QML, and wire its completion signal to the terminal:

```cpp
#include "AutoFillController.h"
// ...
auto* autoFill = new dante::AutoFillController(&app);
engine.rootContext()->setContextProperty("autoFill", autoFill);

// Route raw commands (no placeholders) from PaletteController through the
// same path so QML only listens on one entry point.
QObject::connect(palette, &dante::PaletteController::autoFillRequested,
                 autoFill, &dante::AutoFillController::prepare);

// Render result → terminal write on the active tab. Same path the palette
// uses for `terminalWriteRequested` (TerminalBridge::write(sessionId, text)).
QObject::connect(autoFill, &dante::AutoFillController::commandReady,
                 [appState, terminal](const QString& text) {
                     const QString tabId = appState->activeTabId();
                     if (!tabId.isEmpty()) terminal->write(tabId, text);
                 });
```

If `TerminalBridge::write` has a different name, use whatever main.cpp already invokes for `PaletteController::terminalWriteRequested`.

## `ui/qml/Main.qml`

Add a single instance of the dialog at the root layer. It opens and closes
automatically based on `autoFill.pendingPlaceholders`, so no shortcut is
required:

```qml
AutoFillDialog {
    id: autoFillDialog
}
```

Place it alongside the other root-level popups (CommandPalette, About, etc.).

## Verification

1. Open the snippets editor and create a snippet with the command:
   `git commit -m "$(Mensagem|text)"`
2. Open the command palette (Ctrl+K), filter to the new snippet, press Enter.
3. The terminal **must NOT** receive the raw template. Instead, the AutoFill
   dialog opens showing one labelled field "Mensagem".
4. Type a message, press Executar — the terminal receives
   `git commit -m "<your message>"` followed by `\r`.
5. Repeat with a placeholder using a default: `echo ${USER:dante}` — the
   field shows `dante` pre-filled. Hitting Executar with the box empty
   substitutes the default.
6. Repeat with `scp $(Arquivo|path) user@host:/tmp/` — clicking "Procurar"
   opens the native FileDialog and fills the field with the chosen path.
