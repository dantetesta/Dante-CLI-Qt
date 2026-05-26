// Top-level window. Three columns:
//   [Sidebar] [Splitter] [Main pane] [optional RightSidebar]
// Bottom: Toolbar full-width under the main pane.
import QtQuick 6.5
import QtQuick.Window 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

ApplicationWindow {
    id: root
    visible: true
    width:  1280
    height: 800
    minimumWidth:  980
    minimumHeight: 600
    title: "Dante CLI"
    color: Theme.ink

    // Persist sidebar width when user resizes.
    property int  sidebarWidth: appState.sidebarWidth

    onClosing: appState.saveOnClose()

    RowLayout {
        id: mainRow
        anchors.fill: parent
        spacing: 0
        opacity: 0
        Component.onCompleted: splashIn.start()

        // First-launch splash: fade the main row in over 200ms.
        NumberAnimation {
            id: splashIn
            target: mainRow
            property: "opacity"
            from: 0; to: 1
            duration: 200
            easing.type: Easing.OutCubic
        }

        // ───── Left sidebar ─────
        Sidebar {
            id: sidebar
            Layout.preferredWidth: root.sidebarWidth
            Layout.minimumWidth:   220
            Layout.maximumWidth:   560
            Layout.fillHeight:     true

            // Animate programmatic width changes (collapse toggle, snap, etc).
            // Drag updates pass through every onPositionChanged tick so the
            // splitter still feels 1:1 with the mouse.
            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
        }

        // ───── Splitter ─────
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: splitterArea.pressed
                       ? Theme.accent
                       : (splitterArea.containsMouse ? Theme.borderStrong : Theme.borderSoft)
            Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
            MouseArea {
                id: splitterArea
                anchors.fill: parent
                anchors.leftMargin: -3; anchors.rightMargin: -3
                hoverEnabled: true
                cursorShape: Qt.SplitHCursor
                property real startX
                property int  startW
                onPressed: { startX = mouseX; startW = root.sidebarWidth; }
                onPositionChanged: {
                    if (pressed) {
                        const next = Math.max(220, Math.min(560, startW + (mouseX - startX)))
                        root.sidebarWidth = next
                        appState.sidebarWidth = next
                    }
                }
            }
        }

        // ───── Main column ─────
        ColumnLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: 0

            // Autoupdate banner — sits above the tab bar; collapses when no
            // update is available so it doesn't steal vertical space.
            UpdateBanner {
                id: updateBanner
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 44 : 0
                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: Theme.motionStd; easing.type: Easing.OutCubic }
                }
            }

            TabBar {
                Layout.fillWidth: true
            }

            // The active terminal pane fills the remaining space.
            Terminal {
                id: term
                Layout.fillWidth:  true
                Layout.fillHeight: true
            }

            BottomToolbar {
                Layout.fillWidth: true
            }
        }
    }

    // ───── AI overlay ─────
    // Floats above the main layout (z stacking handled inside AIOverlay).
    // Closes on outside-click; AIController owns the visibility state.
    AIOverlay {
        id: aiOverlay
        anchors.fill: parent
        panelWidth: 420
        toolbarHeight: 42
    }

    // ───── Command palette (Ctrl+K) ─────
    // Owns its own scrim + scale-in animation. PaletteController owns state.
    CommandPalette {
        id: commandPalette
        anchors.fill: parent
    }

    // ───── Voice HUD ─────
    // Pulsing dot + waveform while voice.status != "idle". Sits below the
    // AI overlay (z 950 vs 1000) so the chat modal still wins focus if
    // both happen to be visible at once.
    VoiceWidget {
        id: voiceHud
        anchors.fill: parent
    }

    // ───── Split-layout designer ─────
    // Centered modal. Opened from the BottomToolbar ▦ button; emits
    // `applied()` when the user clicks Aplicar.
    LayoutDesigner {
        id: layoutDesigner
        anchors.centerIn: parent
    }

    // ───── Settings panel ─────
    SettingsPanel {
        id: settingsPanel
        anchors.centerIn: parent
    }

    // Wire AI → terminal injection.
    Connections {
        target: ai
        function onInsertRequested(text) {
            if (!appState.activeTabId) return
            terminal.write(appState.activeTabId, text)
        }
    }

    // Wire Voice → terminal injection. If the AI overlay is open when the
    // transcript arrives we forward it to the prompt input instead — the
    // user clearly wanted to dictate to the assistant in that case.
    Connections {
        target: voice
        function onTranscribed(text) {
            if (ai && ai.open) {
                ai.send(text)
            } else if (appState.activeTabId) {
                terminal.write(appState.activeTabId, text)
            }
        }
    }

    // Wire palette → terminal write requests (cd to favorite, snippet, etc.).
    // ignoreUnknownSignals silences qmlcache warnings about not being able to
    // resolve the signal types of a context-property target ahead of time.
    Connections {
        target: palette
        ignoreUnknownSignals: true
        function onTerminalWriteRequested(sessionId, text) {
            terminal.write(sessionId, text)
        }
        function onClearRequested(sessionId) {
            // Send the canonical clear sequence (ANSI ED 2J + cursor home).
            terminal.write(sessionId, "[2J[H")
        }
        function onAiToggleRequested() { ai.toggle() }
        function onFocusTerminalRequested() { term.forceActiveFocus() }
        function onVoiceStartRequested() {
            // Palette → start dictation. Toggle so re-triggering the same
            // palette entry while a take is open will end it.
            if (typeof voice !== "undefined" && voice) voice.toggleRecording()
        }
    }

    // Global ⌘T / Ctrl+T → new tab. `sequences` (plural) binds all of
    // QKeySequence::AddTab's platform mappings, not just the primary.
    Shortcut { sequences: [StandardKey.AddTab]; onActivated: appState.addTab("Terminal") }
    Shortcut { sequences: [StandardKey.Close];  onActivated: appState.closeTab(appState.activeTabId) }
    Shortcut { sequence: "Ctrl+Shift+]";       onActivated: term.cycleTab(1) }
    Shortcut { sequence: "Ctrl+Shift+[";       onActivated: term.cycleTab(-1) }
    // Toggle AI overlay with Ctrl+Shift+A (matches Swift sibling vibe).
    Shortcut { sequence: "Ctrl+Shift+A";       onActivated: ai.toggle() }
    // Command palette: Ctrl+K (Spotlight / VS Code Cmd+P style).
    Shortcut { sequence: "Ctrl+K";             onActivated: palette.toggle() }

    // ─── Split-pane shortcuts ───────────────────────────────────────────
    // Vertical = side-by-side (Ctrl+\), horizontal = stacked (Ctrl+Shift+\).
    // Close focused pane = Ctrl+Shift+W. Focus left/right between two panes
    // with Ctrl+Alt+← / →. All ops delegate to the active SplitContainer.
    Shortcut { sequence: "Ctrl+\\";       onActivated: appState.splitActive("vertical") }
    Shortcut { sequence: "Ctrl+Shift+\\"; onActivated: appState.splitActive("horizontal") }
    Shortcut {
        sequence: "Ctrl+Shift+W"
        onActivated: {
            const sc = term.splitContainer
            if (sc && sc.isSplit) sc.closePane(sc.focusedSide)
        }
    }
    Shortcut {
        sequence: "Ctrl+Alt+Left"
        onActivated: { if (term.splitContainer.isSplit) term.splitContainer.focusSide("a") }
    }
    Shortcut {
        sequence: "Ctrl+Alt+Right"
        onActivated: { if (term.splitContainer.isSplit) term.splitContainer.focusSide("b") }
    }
}
