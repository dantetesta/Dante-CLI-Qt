# SPEC-070 — Right Sidebar (Skills / Agents / MCPs) — Integration Notes

This batch is self-contained except for three small wirings — none touch
the forbidden files (`AppState`, `main.cpp`, `TerminalBridge`, `Main.qml`,
`Theme.qml`, `BottomToolbar.qml`, `Sidebar.qml`) in their existing
contracts but they DO need additions in those files. The diffs below are
the only edits a human needs to apply.

## CMakeLists.txt additions

### `core/CMakeLists.txt`

Add the new sources to the `dante-core` STATIC library list. Qt6::Concurrent
is already linked — no link-line change needed.

```cmake
add_library(dante-core STATIC
    # … existing entries …
    claude/ClaudeResources.h
    claude/ClaudeResourceScanner.cpp
    claude/ClaudeResourceScanner.h
)
```

### `apps/desktop-qt/CMakeLists.txt`

Two patches: register the controller as a source, and add the five new
QML files to the QML module + alias list.

```cmake
qt_add_executable(dante-cli WIN32
    # … existing entries …
    src/ResourcesController.cpp
    src/ResourcesController.h
)

# Inside the first foreach() that sets QT_RESOURCE_ALIAS for root QMLs:
foreach(qml_file Main TabBar TabChip Sidebar Terminal BottomToolbar Theme
                 AIOverlay AIMessage SchemePicker
                 PaneView SplitContainer CommandPalette VoiceWidget
                 UpdateBanner LayoutDesigner SettingsPanel
                 EmptySlot VideoOpenDialog GridWorkspace FileTreeView
                 MovablePopup CheatsheetView AboutView EditorView
                 RecursiveSplit
                 RightSidebar SkillsTab AgentsTab MCPsTab ResourceRow)
    # … unchanged body …
endforeach()

qt_add_qml_module(dante-cli
    # … existing entries …
    QML_FILES
        # … existing list …
        ../../ui/qml/RightSidebar.qml
        ../../ui/qml/SkillsTab.qml
        ../../ui/qml/AgentsTab.qml
        ../../ui/qml/MCPsTab.qml
        ../../ui/qml/ResourceRow.qml
)
```

## main.cpp additions

Add the include, instantiate the controller, hydrate it, wire its
`requestTerminalWrite` signal to the existing terminal bridge, and
expose it as the `resources` QML context property.

```cpp
#include "ResourcesController.h"
// …
auto* resources = new dante::ResourcesController(&app);
resources->hydrate();
// If a project cwd is known (when AppState gains it), thread it
// through here. For now the scanner works user-scope only.
// resources->setProjectCwd(appState->activeTabCwd());

QObject::connect(resources, &dante::ResourcesController::requestTerminalWrite,
                 [appState, terminal](const QString& text) {
                     if (!appState->activeTabId().isEmpty())
                         terminal->write(appState->activeTabId(), text);
                 });

engine.rootContext()->setContextProperty("resources", resources);
```

## Main.qml additions

`appState.rightSidebarVisible` already exists (verified in
`AppState.h` line 17) and `Ctrl+]` is already wired at line 336–339.
Add the dock to the top-level `RowLayout` after the main `ColumnLayout`:

```qml
// ───── Right sidebar ─────
Rectangle {
    Layout.preferredWidth: appState.rightSidebarVisible && !root.focusMode ? 320 : 0
    Layout.fillHeight: true
    color: "transparent"
    visible: Layout.preferredWidth > 0
    clip: true
    Behavior on Layout.preferredWidth {
        NumberAnimation { duration: Theme.motionStd; easing.type: Easing.OutCubic }
    }
    RightSidebar {
        anchors.fill: parent
    }
}
```

No new shortcuts needed — `Ctrl+]` is in place.

## BottomToolbar.qml additions (optional)

Add a toggle icon mirroring the left-sidebar collapse button:

```qml
// Pseudo-snippet — drop next to the existing right-aligned buttons.
Rectangle {
    Layout.preferredWidth: 28
    Layout.preferredHeight: 28
    radius: Theme.radiusSm
    color: rsArea.containsMouse ? Theme.surfaceHigh : "transparent"
    border.color: appState.rightSidebarVisible ? Theme.accentSoft : Theme.borderSoft
    Text {
        anchors.centerIn: parent
        text: "⟩"
        color: appState.rightSidebarVisible ? Theme.accent : Theme.fgMuted
        font.pixelSize: 13
    }
    MouseArea {
        id: rsArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: appState.rightSidebarVisible = !appState.rightSidebarVisible
        ToolTip.text: qsTr("Skills / Agents / MCPs (Ctrl+])")
        ToolTip.visible: containsMouse
    }
}
```

## Verification

1. `cmake --build build` cleanly — Qt6::Concurrent is already linked.
2. Launch `dante-cli`; press **Ctrl+]** — the right dock slides in.
3. **Skills tab** lists every `<dir>/SKILL.md` under `~/.claude/skills/`.
   Click a row -> the active terminal receives `/<name> ` (slash + name
   + trailing space).
4. **Agents tab** lists every `*.md` under `~/.claude/agents/`. Click ->
   same slash-injection.
5. **MCPs tab** lists each entry under `~/.mcp.json`'s `mcpServers`
   block. Click -> the name is injected as-is (no slash) so the user
   can wrap it with `@` or pass it to a tool.
6. Edit a SKILL.md in another window, save -> within 300ms the row's
   description refreshes (`QFileSystemWatcher` + debounced rescan).
7. Refresh button (↻) re-arms an immediate rescan.
8. Search field filters all three tabs by name + description (client-side).

## Known follow-ups (out of scope for this batch)

- Project-scope scanning currently requires explicitly calling
  `resources.setProjectCwd(...)`. Once `AppState` exposes
  `activeTabCwd()` (sibling parity), wire it through in `main.cpp` and
  re-emit on `cwdChanged` so the list reflects the focused tab.
- Right-click context menu (open file, copy name, delete) is the next
  batch — the Swift sibling has it, but the spec scope here is just
  list + click-to-insert.
- `~/.claude.json` per-project `mcpServers` aren't scanned yet — the
  Swift sibling reads them when `activeProjectCWD` is set; gated on
  the cwd plumbing above.
