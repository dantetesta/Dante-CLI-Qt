# SPEC-022 — Browser pane via QtWebEngine

New files (already authored):
- `ui/qml/BrowserView.qml`

The pane is wired into `Terminal.qml` via a new `useBrowser` loader branch
keyed on `TabKind::Browser` (= 4). Persistence is one `browserUrl` field on
`Tab`, serialised through the existing `tabToJson` / `tabFromJson` pair.

QtWebEngine is heavy (≈150 MB on Windows after deployment) — that's the
known cost of embedding Chromium and we accept it.

---

## 1. CMakeLists.txt

### Root `CMakeLists.txt`

Add `WebEngineQuick` to the existing `find_package` line:

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Network Sql Concurrent Widgets
    Multimedia MultimediaWidgets
    WebEngineQuick
)
```

### `apps/desktop-qt/CMakeLists.txt`

Add `BrowserView` to the alias loop (after `VideoView`):

```cmake
foreach(qml_file Main TabBar TabChip Sidebar Terminal BottomToolbar Theme
                 AIOverlay AIMessage SchemePicker
                 PaneView SplitContainer CommandPalette VoiceWidget
                 UpdateBanner LayoutDesigner SettingsPanel
                 EmptySlot VideoOpenDialog GridWorkspace FileTreeView
                 MovablePopup CheatsheetView AboutView EditorView
                 RecursiveSplit CalculatorView VideoView BrowserView
                 AIProvidersTab
                 RightSidebar SkillsTab AgentsTab MCPsTab ResourceRow
                 AutoFillDialog GeneratorsPalette)
```

Add the QML file to `QML_FILES` (after the `VideoView.qml` line):

```cmake
        ../../ui/qml/BrowserView.qml
```

Link `Qt6::WebEngineQuick`:

```cmake
target_link_libraries(dante-cli PRIVATE
    dante-core
    dante-infra
    Qt6::Quick Qt6::QuickControls2 Qt6::Widgets
    Qt6::Multimedia
    Qt6::WebEngineQuick
)
```

---

## 2. `apps/desktop-qt/src/main.cpp`

`QtWebEngineQuick::initialize()` MUST run before the `QApplication`
constructor — Qt enforces this so Chromium can install its argv hooks and
GPU/IPC plumbing on the main thread.

```cpp
#include <QtWebEngineQuick>

int main(int argc, char* argv[]) {
    QtWebEngineQuick::initialize();
    // The rest of the existing main(): QApplication a(argc, argv); …
}
```

No context-property additions are needed — `BrowserView.qml` only reads
the existing `appState` global.

---

## 3. `core/domain/Models.h`

`TabKind::Browser` is already in the enum. Add the per-tab URL field on
`Tab`:

```cpp
struct Tab {
    // … existing fields …

    // SPEC-022 — Browser tab content. Only meaningful when kind == Browser.
    QString browserUrl;    // empty = blank tab; otherwise a full URL
};
```

---

## 4. `apps/desktop-qt/src/AppState.h`

Append after the SPEC-023 block:

```cpp
/* ─── SPEC-022 — Browser tab kind ─── */
Q_INVOKABLE QString newBrowserTab(const QString& title, const QString& url);
Q_INVOKABLE QString tabBrowserUrl(const QString& tabId) const;
Q_INVOKABLE void    setTabBrowserUrl(const QString& tabId, const QString& url);
```

---

## 5. `apps/desktop-qt/src/AppState.cpp`

### 5a. `tabToJson` / `tabFromJson`

```cpp
QJsonObject tabToJson(const Tab& t) {
    return {
        // … existing keys …
        {"videoPath",   t.videoPath},
        {"browserUrl",  t.browserUrl},
    };
}

Tab tabFromJson(const QJsonObject& o) {
    Tab t;
    // … existing assignments …
    t.videoPath   = o.value("videoPath").toString();
    t.browserUrl  = o.value("browserUrl").toString();
    return t;
}
```

### 5b. Implementations

```cpp
QString AppState::newBrowserTab(const QString& title, const QString& url) {
    Tab t;
    t.id         = newId();
    t.kind       = TabKind::Browser;
    t.title      = title.isEmpty() ? QStringLiteral("Navegador") : title;
    t.emoji      = QStringLiteral("🌐");
    t.color      = QStringLiteral("#7C82FF");
    t.browserUrl = url;
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}

QString AppState::tabBrowserUrl(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].browserUrl;
}

void AppState::setTabBrowserUrl(const QString& tabId, const QString& url) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    Tab& t = tabs_[i];
    if (t.browserUrl == url) return;
    t.browserUrl = url;
    emit tabsChanged();
    persistSession();
}
```

> `tabKind` already returns the int, so `appState.tabKind(id) === 4`
> (= `TabKind::Browser`) is the QML check used by `Terminal.qml` below.

---

## 6. `ui/qml/Terminal.qml`

Add a `useBrowser` branch and a `browserLoader`. Patch the readonly chain:

```qml
readonly property int  kind: (_bump, appState.tabKind(appState.activeTabId))
readonly property bool useEditor:     kind === 1
readonly property bool useVideo:      kind === 3   // TabKind::Video
readonly property bool useBrowser:    kind === 4   // TabKind::Browser
readonly property bool useCalculator: (_bump,
    appState.tabKindString(appState.activeTabId) === "calculator")
readonly property bool useTree:   !useEditor && !useVideo && !useBrowser && !useCalculator
    && (… existing tree probe …)
readonly property bool useGrid:   !useEditor && !useVideo && !useBrowser && !useTree && !useCalculator
    && (… existing grid probe …)
readonly property bool useSplit:  !useEditor && !useVideo && !useBrowser && !useTree && !useGrid && !useCalculator
```

Add the loader (place after `videoLoader`):

```qml
Loader {
    id: browserLoader
    anchors.fill: parent
    active: root.useBrowser
    sourceComponent: BrowserView {}
}
```

---

## 7. Verification

1. Build cleanly on macOS (`cmake --build build`). QtWebEngine is the only
   new dependency; first build is slow because Chromium libs are linked.
2. From the command palette (or directly in a QML console) call
   `appState.newBrowserTab("Example", "https://example.com")`. The tab
   pane swaps to `BrowserView` and the page loads.
3. Click ‹ / › to navigate history; ⟳ reloads, ✕ stops while loading.
4. Type a new URL in the bar and press Enter — bare hostnames are
   auto-prefixed with `https://`, free-form queries fall through to
   DuckDuckGo.
5. Drag a link from another browser window onto the body or onto the URL
   bar — the tab navigates.
6. Right-click anywhere in the page → "Copiar URL", "Recarregar",
   "Abrir no navegador externo".
7. Close the app, reopen — the last URL re-hydrates in the same tab via
   `browserUrl` round-tripping through `session.json`.
