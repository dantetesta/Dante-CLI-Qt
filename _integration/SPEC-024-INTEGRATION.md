# SPEC-024 — Calculator pane + engine

This patch wires the new C++ engine, controller and QML view into the
existing scene. Files under `core/calculator/`,
`apps/desktop-qt/src/CalculatorController.{h,cpp}` and
`ui/qml/CalculatorView.qml` are already written; the snippets below cover
every file you must edit by hand.

## CMakeLists.txt additions

### `core/CMakeLists.txt`

Add the engine sources to the `dante-core` static library:

```diff
 add_library(dante-core STATIC
     persistence/JsonStore.cpp
     persistence/JsonStore.h
     persistence/AppPaths.cpp
     persistence/AppPaths.h
+    calculator/CalculatorEngine.cpp
+    calculator/CalculatorEngine.h
     terminal/TerminalSession.cpp
     ...
 )
```

### `apps/desktop-qt/CMakeLists.txt`

1. Add the controller to `qt_add_executable`:

```diff
 qt_add_executable(dante-cli WIN32
     src/main.cpp
     src/AppState.cpp
     src/AppState.h
     ...
     src/FileTreeController.cpp
     src/FileTreeController.h
+    src/CalculatorController.cpp
+    src/CalculatorController.h
 )
```

2. Register the QML alias loop entry (preserves the relative path used by
   `qt_add_qml_module`):

```diff
 foreach(qml_file Main TabBar TabChip Sidebar Terminal BottomToolbar Theme
                  AIOverlay AIMessage SchemePicker
                  PaneView SplitContainer CommandPalette VoiceWidget
                  UpdateBanner LayoutDesigner SettingsPanel
                  EmptySlot VideoOpenDialog GridWorkspace FileTreeView
                  MovablePopup CheatsheetView AboutView EditorView
-                 RecursiveSplit)
+                 RecursiveSplit CalculatorView)
```

3. Add the QML to `QML_FILES` in `qt_add_qml_module`:

```diff
 qt_add_qml_module(dante-cli
     URI dante.ui
     VERSION 1.0
     QML_FILES
         ...
         ../../ui/qml/RecursiveSplit.qml
+        ../../ui/qml/CalculatorView.qml
         ../../ui/qml/editors/EmojiPicker.qml
         ...
 )
```

## `core/domain/Models.h`

Extend `TabKind` so a tab can be tagged as a calculator. Keep enum
values stable — append `Calculator` last to avoid shifting persisted
session.json data.

```diff
-enum class TabKind { Terminal, Editor, Preview, Video, Browser };
+enum class TabKind { Terminal, Editor, Preview, Video, Browser, Calculator };
```

## `AppState.h` additions

Add a way to create the new tab kind and a string-form `tabKind` helper
that Terminal.qml will switch on (mirrors the SPEC's "calculator" string
contract without exposing the enum to QML).

```diff
     Q_INVOKABLE int     tabKind(const QString& tabId) const; // 0=Terminal, 1=Editor, …
+    /// String form (e.g. "terminal", "editor", "calculator") for QML routing.
+    Q_INVOKABLE QString tabKindString(const QString& tabId) const;
+
+    /// SPEC-024 — open (or focus existing) calculator tab.
+    Q_INVOKABLE QString openCalculatorTab();
```

## `AppState.cpp` additions

```cpp
QString AppState::tabKindString(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return QStringLiteral("terminal");
    switch (tabs_[i].kind) {
        case TabKind::Terminal:   return QStringLiteral("terminal");
        case TabKind::Editor:     return QStringLiteral("editor");
        case TabKind::Preview:    return QStringLiteral("preview");
        case TabKind::Video:      return QStringLiteral("video");
        case TabKind::Browser:    return QStringLiteral("browser");
        case TabKind::Calculator: return QStringLiteral("calculator");
    }
    return QStringLiteral("terminal");
}

QString AppState::openCalculatorTab() {
    // Re-use an existing calculator tab — there's only ever one engine,
    // so multiple tabs would just mirror each other.
    for (const auto& t : tabs_) {
        if (t.kind == TabKind::Calculator) {
            activeTabId_ = t.id;
            emit activeTabIdChanged();
            return t.id;
        }
    }
    Tab t;
    t.id     = newId();
    t.kind   = TabKind::Calculator;
    t.title  = QStringLiteral("Calculadora");
    t.emoji  = QStringLiteral("🧮");
    t.color  = QStringLiteral("#7C82FF");
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}
```

## `main.cpp` additions

```diff
 #include "FileTreeController.h"
+#include "CalculatorController.h"
```

```diff
     auto* fileTree   = new dante::FileTreeController(&app);
+    auto* calculator = new dante::CalculatorController(&app);
+    calculator->hydrate();
```

```diff
     engine.rootContext()->setContextProperty("fileTree",   fileTree);
+    engine.rootContext()->setContextProperty("calculator", calculator);
```

## `Main.qml` additions

Optional global shortcut to open the calculator (mirrors palette entries):

```qml
Shortcut {
    sequences: ["Ctrl+Shift+K"]   // Cmd on macOS
    onActivated: appState.openCalculatorTab()
}
```

You can also append a row to `PaletteController::rebuildCatalog` so it
shows up as a palette command — same pattern as the existing scheme
rows.

## `Terminal.qml` routing

Add a calculator branch to the loader cascade:

```diff
     readonly property int  kind: (_bump, appState.tabKind(appState.activeTabId))
     readonly property bool useEditor: kind === 1
+    readonly property bool useCalculator: (_bump,
+        appState.tabKindString(appState.activeTabId) === "calculator")
     readonly property bool useTree: !useEditor
+        && !useCalculator
         && (_bump, !appState.tabPaneTree(appState.activeTabId).leaf
                   && !appState.tabPaneTree(appState.activeTabId).split
                   ? false : true)
     readonly property bool useGrid: !useEditor && !useTree
+        && !useCalculator
         && (_bump, appState.tabGridCols(appState.activeTabId) > 0
                   && appState.tabGridRows(appState.activeTabId) > 0)
-    readonly property bool useSplit: !useEditor && !useTree && !useGrid
+    readonly property bool useSplit: !useEditor && !useTree && !useGrid && !useCalculator
```

```diff
     Loader {
         id: editorLoader
         anchors.fill: parent
         active: root.useEditor
         sourceComponent: EditorView {}
     }
+
+    Loader {
+        id: calculatorLoader
+        anchors.fill: parent
+        active: root.useCalculator
+        sourceComponent: CalculatorView {}
+    }
```

## Verification

- Build clean on macOS (`cmake --build build`) and Windows (MSVC).
- Open via shortcut (Ctrl+Shift+K) or by calling
  `appState.openCalculatorTab()` from the command palette / QML console.
- Smoke tests:
  - `2 + 3 * 4` → `14`
  - `sin(pi / 2)` → `1`
  - `sqrt(16) + ln(e)` → `5`
  - `1 / 0` → red error band.
  - Type a digit, press `M+`, clear, press `MR` → display restored.
  - Click a row in the history panel → expression re-loads and re-evals.
- Confirm persistence: evaluate a few expressions, kill the app, relaunch
  and check that the history list is still populated.
  File lives at `<AppData>/Dante Testa/Dante CLI/calculator-history.json`.
