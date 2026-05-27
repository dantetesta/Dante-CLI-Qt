# SPEC-060 — Process stats indicator (integration)

This SPEC adds a discreet `CPU 12% · 142 MB` indicator to the BottomToolbar,
backed by a cross-platform sampler (`dante::system::ProcessStatsSampler`)
and a QML-facing controller (`dante::ProcessStatsController`).

## Files added by this SPEC

- `core/system/ProcessStats.h`
- `core/system/ProcessStats.cpp`
- `apps/desktop-qt/src/ProcessStatsController.h`
- `apps/desktop-qt/src/ProcessStatsController.cpp`

## CMakeLists.txt additions

### `core/CMakeLists.txt`

Add the two new sources to the `add_library(dante-core STATIC …)` source
list, alongside the other `<subdir>/<file>.{cpp,h}` entries:

```cmake
add_library(dante-core STATIC
    # … existing sources …
    system/ProcessStats.cpp
    system/ProcessStats.h
)
```

Then, **after** the existing `target_link_libraries(dante-core PUBLIC …)`
block, append the Windows-only PSAPI link:

```cmake
if(WIN32)
    # GetProcessMemoryInfo() lives in psapi on MSVC.
    target_link_libraries(dante-core PUBLIC psapi)
endif()
```

No extra linkage is needed on macOS — `task_info` is in the platform
runtime and is pulled in automatically. Linux only needs libc.

### `apps/desktop-qt/CMakeLists.txt`

Inside the `qt_add_executable(dante-cli WIN32 …)` source list, add the
controller next to the other `src/*Controller.{cpp,h}` entries:

```cmake
qt_add_executable(dante-cli WIN32
    # … existing sources …
    src/ProcessStatsController.cpp
    src/ProcessStatsController.h
)
```

No QML file additions are required: the indicator lives inside the
existing `BottomToolbar.qml`.

## `main.cpp` additions

1. Add the include near the other controllers:

   ```cpp
   #include "ProcessStatsController.h"
   ```

2. Instantiate the controller right after `auto* fileTree = new …;`
   (block around lines 73-76 in the current `main.cpp`):

   ```cpp
   auto* processStats = new dante::ProcessStatsController(&app);
   ```

3. Register the QML context property next to the other
   `engine.rootContext()->setContextProperty(…)` calls (around line 132):

   ```cpp
   engine.rootContext()->setContextProperty("processStats", processStats);
   ```

No `AppState` wiring is needed — the controller is self-contained and
internally drives its own `QTimer`.

## `BottomToolbar.qml` additions

The current file already has a placeholder `Text { text: "0% · 0 B"; … }`
between the "Layout do split" group and the `Item { Layout.fillWidth: true }`
spacer (lines ~48-55 in alpha.25). Replace that placeholder block with the
snippet below. Keep the `Sep {}` that precedes it.

**Replace** these lines:

```qml
Sep {}
// CPU / mem stats — placeholder for now (Swift sibling tracks real values).
Text {
    text: "0% · 0 B"
    color: Theme.fgFaint
    font.family: Theme.fontMono
    font.pixelSize: 11
    Layout.alignment: Qt.AlignVCenter
}
```

**with** (placement: between the "Layout do split" `ToolbarButton` group
and the `Item { Layout.fillWidth: true }` spacer — same spot the
placeholder used):

```qml
Sep {}

// CPU / mem stats — wired to dante::ProcessStatsController.
// Monospace digits + muted color so it reads as ambient telemetry, not
// an action. Hover tooltip surfaces precise values + sampling interval.
Rectangle {
    id: statsPill
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 22
    Layout.preferredWidth: statsRow.implicitWidth + 14
    radius: Theme.radiusSm
    color: statsHover.containsMouse ? Theme.surfaceHigh : "transparent"
    border.color: statsHover.containsMouse ? Theme.borderSoft : "transparent"
    border.width: 1
    Behavior on color       { ColorAnimation { duration: Theme.motionFast } }
    Behavior on border.color{ ColorAnimation { duration: Theme.motionFast } }

    Row {
        id: statsRow
        anchors.centerIn: parent
        spacing: 6
        Text {
            text: (typeof processStats !== "undefined" && processStats)
                  ? ("CPU " + Math.round(processStats.cpuPercent) + "%")
                  : "CPU 0%"
            color: Theme.fgMuted
            font.family: Theme.fontMono
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "·"
            color: Theme.fgFaint
            font.family: Theme.fontMono
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: (typeof processStats !== "undefined" && processStats)
                  ? (Math.round(processStats.memoryMB) + " MB")
                  : "0 MB"
            color: Theme.fgMuted
            font.family: Theme.fontMono
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: statsHover
        anchors.fill: parent
        hoverEnabled: true
        ToolTip.text: (typeof processStats !== "undefined" && processStats)
                      ? processStats.tooltipText
                      : ""
        ToolTip.visible: containsMouse
                         && (typeof processStats !== "undefined" && processStats)
        ToolTip.delay: 400
    }
}
```

### Theme token note

The original SPEC referenced `Theme.textMuted`, but the project's current
`Theme.qml` exposes `fgMuted` / `fgFaint` (text scale) instead. The snippet
above uses the real tokens that exist in the singleton. If `Theme.textMuted`
is later introduced as an alias, swap the two `color: Theme.fgMuted` lines.

### Placement guidance

Insert the snippet **between** the "Layout do split" `ToolbarButton` group
(around line 46) and the `Item { Layout.fillWidth: true }` spacer (around
line 57). It is a drop-in replacement for the placeholder Text node that
currently lives there — no other parts of `BottomToolbar.qml` need to move.

## Verification

- **macOS**: launch `Dante CLI`, hover the pill in the bottom toolbar after
  ~2 seconds. CPU should settle to a low single-digit percentage while idle
  and tick up under typing/animation load. Memory should match what
  Activity Monitor's "Real Memory" column shows for the same process,
  within a few MB.
- **Windows**: same flow. CPU% comes from `GetProcessTimes` deltas, RSS
  from `GetProcessMemoryInfo().WorkingSetSize` — both match Task Manager's
  "CPU" and "Memory (Working Set)" columns for the `Dante CLI.exe` row.
- **Tooltip**: hovering shows e.g.
  `CPU sampling: 1s · RSS: 142.0 MB · CPU: 12.3%`.
- **Jitter**: emits are suppressed for moves < 0.5% CPU / < 1 MB so the
  text shouldn't flicker every tick when idle.
- **Tuning**: call `processStats.setSamplingInterval(2000)` from the QML
  console (or a future settings switch) to slow the timer to 2 s without
  rebuilding.
