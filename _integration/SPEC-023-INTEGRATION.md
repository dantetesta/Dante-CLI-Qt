# SPEC-023 — Video pane integration

New files (already authored):
- `ui/qml/VideoView.qml`

The pane is wired into the existing `Terminal.qml` Loader switch and a new
`TabKind::Video` value in `core/domain/Models.h`. AppState gains a small
`videoPath` accessor surface plus a `newVideoTab(...)` helper for the
"Open in slot" / drag-into-tab-bar entrypoints.

---

## 1. CMakeLists.txt

### Root `CMakeLists.txt`

`Multimedia` is already in the root `find_package` line. Extend it with
`MultimediaWidgets` so QtMultimedia's QML side picks up the VideoOutput
backend on every platform:

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Network Sql Concurrent Widgets
    Multimedia MultimediaWidgets
)
```

### `apps/desktop-qt/CMakeLists.txt`

Add `VideoView` to the alias loop and to `QML_FILES`, and link the
multimedia targets.

In the alias loop (line ~37), insert `VideoView` after `VideoOpenDialog`:

```cmake
foreach(qml_file Main TabBar TabChip Sidebar Terminal BottomToolbar Theme
                 AIOverlay AIMessage SchemePicker
                 PaneView SplitContainer CommandPalette VoiceWidget
                 UpdateBanner LayoutDesigner SettingsPanel
                 EmptySlot VideoOpenDialog VideoView GridWorkspace FileTreeView
                 MovablePopup CheatsheetView AboutView EditorView
                 RecursiveSplit)
```

In `QML_FILES` (line ~63), add the file (preserving the existing alias):

```cmake
        ../../ui/qml/VideoView.qml
```

In `target_link_libraries` add `Qt6::Multimedia` and `Qt6::MultimediaQuick`:

```cmake
target_link_libraries(dante-cli PRIVATE
    dante-core
    dante-infra
    Qt6::Quick Qt6::QuickControls2 Qt6::Widgets
    Qt6::Multimedia Qt6::MultimediaQuick
)
```

---

## 2. `core/domain/Models.h`

The enum already declares `Video` — no change needed. Add the path field
on `Tab`:

```cpp
struct Tab {
    // … existing fields …

    // SPEC-023 — Video tab content. Only meaningful when kind == Video.
    QString videoPath;     // absolute path or file:// url
};
```

---

## 3. `apps/desktop-qt/src/AppState.h`

Inside the `SPEC-021 — Editor tab kind` block (or just below it), append:

```cpp
/* ─── SPEC-023 — Video tab kind ─── */
Q_INVOKABLE QString newVideoTab(const QString& title, const QString& path);
Q_INVOKABLE QString tabVideoPath(const QString& tabId) const;
Q_INVOKABLE void    setTabVideoPath(const QString& tabId, const QString& path);
```

---

## 4. `apps/desktop-qt/src/AppState.cpp`

### 4a. `tabToJson` / `tabFromJson`

Persist `videoPath` alongside the other tab fields:

```cpp
QJsonObject tabToJson(const Tab& t) {
    return {
        // … existing keys …
        {"editorDirty",   t.editorDirty},
        {"paneTree",      paneTreeToJson(t.paneTree)},
        {"videoPath",     t.videoPath},
    };
}

Tab tabFromJson(const QJsonObject& o) {
    Tab t;
    // … existing assignments …
    t.editorDirty     = o.value("editorDirty").toBool();
    t.paneTree        = paneTreeFromJson(o.value("paneTree"));
    t.videoPath       = o.value("videoPath").toString();
    return t;
}
```

### 4b. Implementations

```cpp
QString AppState::newVideoTab(const QString& title, const QString& path) {
    Tab t;
    t.id        = newId();
    t.kind      = TabKind::Video;
    t.title     = title.isEmpty() ? QStringLiteral("Vídeo") : title;
    t.emoji     = QStringLiteral("🎬");
    t.color     = QStringLiteral("#7C82FF");
    t.videoPath = path;
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}

QString AppState::tabVideoPath(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].videoPath;
}

void AppState::setTabVideoPath(const QString& tabId, const QString& path) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    Tab& t = tabs_[i];
    if (t.videoPath == path) return;
    t.videoPath = path;
    emit tabsChanged();
    persistSession();
}
```

> `tabKind` already returns the int, so `appState.tabKind(id) === 3`
> (= `TabKind::Video`) is the QML check used by `Terminal.qml` below.

---

## 5. `ui/qml/Terminal.qml`

Add a `useVideo` branch and a `videoLoader`. Patch:

```qml
readonly property int  kind: (_bump, appState.tabKind(appState.activeTabId))
readonly property bool useEditor: kind === 1
readonly property bool useVideo:  kind === 3   // TabKind::Video
readonly property bool useTree:   !useEditor && !useVideo && (… existing …)
readonly property bool useGrid:   !useEditor && !useVideo && !useTree && (… existing …)
readonly property bool useSplit:  !useEditor && !useVideo && !useTree && !useGrid
```

And add the loader after `editorLoader`:

```qml
Loader {
    id: videoLoader
    anchors.fill: parent
    active: root.useVideo
    sourceComponent: VideoView {}
}
```

---

## 6. Verification

1. Build and launch.
2. From a terminal tab, run a command palette entry that calls
   `appState.newVideoTab("Test", "/path/to/sample.mp4")` (or invoke it
   from the QML console). The tab pane swaps to `VideoView` and the file
   starts playing.
3. Drag-and-drop a different `.mp4` onto the video stage — the source
   updates via `setTabVideoPath`.
4. Scrub slider seeks within the timeline; volume slider + mute toggle
   affect audio; fullscreen toggle flips the host window.
5. Close the app, reopen — the same video reappears in the same tab
   thanks to the `videoPath` round-trip in `session.json`.
