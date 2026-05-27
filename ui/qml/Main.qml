// Top-level window. Three columns:
//   [Sidebar] [Splitter] [Main pane] [optional RightSidebar]
// Bottom: Toolbar full-width under the main pane.
import QtQuick 6.5
import QtQuick.Window 6.5
import QtQuick.Controls 6.5
import QtQuick.Dialogs   // SPEC-021 — Cmd+O picks a file to open in editor
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

    // SPEC-142 — focus mode. Hides sidebar + tab bar + bottom toolbar so
    // the terminal owns the whole window. Toggle via the pill (when on) or
    // the ⌘. / Ctrl+. shortcut.
    property bool focusMode: false

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
            visible: !root.focusMode
            Layout.preferredWidth: root.focusMode ? 0 : root.sidebarWidth
            Layout.minimumWidth:   220
            Layout.maximumWidth:   560
            Layout.fillHeight:     true

            // Animate programmatic width changes (collapse toggle, focus
            // mode, snap, etc). Drag updates pass through every
            // onPositionChanged tick so the splitter still feels 1:1 with
            // the mouse.
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
                visible: !root.focusMode
                Layout.preferredHeight: root.focusMode ? 0 : 40
                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: Theme.motionStd; easing.type: Easing.OutCubic }
                }
            }

            // The active terminal pane fills the remaining space.
            Terminal {
                id: term
                Layout.fillWidth:  true
                Layout.fillHeight: true
            }

            BottomToolbar {
                Layout.fillWidth: true
                visible: !root.focusMode
                Layout.preferredHeight: root.focusMode ? 0 : 42
                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: Theme.motionStd; easing.type: Easing.OutCubic }
                }
            }
        }

        // SPEC-070 — Right sidebar (Skills / Agents / MCPs). Width is bound
        // to appState.rightSidebarVisible so the existing Ctrl+] shortcut
        // and BottomToolbar toggle both control it.
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

    // ───── Cheatsheet (Ctrl+/) — SPEC-140 ─────
    CheatsheetView {
        id: cheatsheet
    }

    // ───── About — SPEC-141 ─────
    AboutView {
        id: aboutView
    }

    // ───── AutoFill dialog — SPEC-080 ─────
    AutoFillDialog {
        id: autoFillDialog
    }

    // ───── Generators palette — SPEC-081 ─────
    GeneratorsPalette {
        id: generatorsPalette
    }

    // ───── SPEC-021 — Open file dialog (Ctrl+O) ─────
    FileDialog {
        id: openFileDlg
        title: qsTr("Abrir arquivo")
        nameFilters: [
            qsTr("Texto e código (*.md *.txt *.json *.js *.ts *.py *.rs *.go *.sh *.zsh *.bash *.yaml *.yml *.toml *.html *.css *.cpp *.c *.h *.qml *.swift)"),
            qsTr("Todos (*)")
        ]
        onAccepted: {
            const local = openFileDlg.selectedFile.toString().replace(/^file:\/\//, "")
            appState.openFileInEditor(local)
        }
    }

    // ───── Focus mode "Sair" pill — SPEC-142 ─────
    Rectangle {
        id: focusPill
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 14
        visible: root.focusMode
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: Theme.motionStd; easing.type: Easing.OutCubic } }
        z: 9999
        width: pillLabel.implicitWidth + 28
        height: 28
        radius: 14
        color: pillMa.containsMouse ? Theme.accent : Theme.accentDim
        border.color: Theme.accentSoft
        border.width: 1
        Text {
            id: pillLabel
            anchors.centerIn: parent
            text: "↗  " + qsTr("Sair do foco (⌘.)")
            color: pillMa.containsMouse ? "white" : Theme.accent
            font.family: Theme.fontSans
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
        MouseArea {
            id: pillMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.focusMode = false
        }
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
    // SPEC-140 — cheatsheet popup. Same chord as Swift sibling.
    Shortcut {
        sequences: ["Ctrl+/", "Ctrl+Shift+/", "F1"]
        onActivated: cheatsheet.opened ? cheatsheet.close() : cheatsheet.open()
    }
    // SPEC-142 — focus mode toggle (Cmd+. / Ctrl+.).
    Shortcut { sequence: "Ctrl+.";       onActivated: root.focusMode = !root.focusMode }
    Shortcut { sequence: "Ctrl+Shift+F"; onActivated: root.focusMode = !root.focusMode }
    // SPEC-141 — About window.
    Shortcut {
        sequences: ["F2"]
        onActivated: aboutView.opened ? aboutView.close() : aboutView.open()
    }
    // SPEC-024 — open Calculator tab (single instance, focuses if already open).
    Shortcut { sequence: "Ctrl+Shift+K"; onActivated: appState.openCalculatorTab() }
    // SPEC-081 — Generators palette (Ctrl+G).
    Shortcut { sequence: "Ctrl+G"; onActivated: generatorsPalette.open() }
    // SPEC-021 — Cmd+O open file in editor, Cmd+S save active editor.
    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: openFileDlg.open()
    }
    Shortcut {
        sequences: [StandardKey.Save]
        onActivated: {
            if (appState.tabKind(appState.activeTabId) === 1)
                appState.saveEditor(appState.activeTabId)
        }
    }

    // ─── SPEC-090 batch (closes 091..098) ───
    // Cmd+D / Cmd+Shift+D — Swift-compat aliases for split shortcuts.
    Shortcut { sequence: "Ctrl+D";       onActivated: appState.splitActive("vertical") }
    Shortcut { sequence: "Ctrl+Shift+D"; onActivated: appState.splitActive("horizontal") }
    // Cmd+1..9 — jump to tab N.
    Shortcut { sequence: "Ctrl+1"; onActivated: appState.selectTab(tabs.data(tabs.index(0,0), 0x101)) }
    Shortcut { sequence: "Ctrl+2"; onActivated: { if (tabs.rowCount() >= 2) appState.selectTab(tabs.data(tabs.index(1,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+3"; onActivated: { if (tabs.rowCount() >= 3) appState.selectTab(tabs.data(tabs.index(2,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+4"; onActivated: { if (tabs.rowCount() >= 4) appState.selectTab(tabs.data(tabs.index(3,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+5"; onActivated: { if (tabs.rowCount() >= 5) appState.selectTab(tabs.data(tabs.index(4,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+6"; onActivated: { if (tabs.rowCount() >= 6) appState.selectTab(tabs.data(tabs.index(5,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+7"; onActivated: { if (tabs.rowCount() >= 7) appState.selectTab(tabs.data(tabs.index(6,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+8"; onActivated: { if (tabs.rowCount() >= 8) appState.selectTab(tabs.data(tabs.index(7,0), 0x101)) } }
    Shortcut { sequence: "Ctrl+9"; onActivated: { if (tabs.rowCount() >= 9) appState.selectTab(tabs.data(tabs.index(8,0), 0x101)) } }
    // Sidebar toggles.
    Shortcut {
        sequence: "Ctrl+Shift+S"
        onActivated: root.sidebarWidth = (root.sidebarWidth > 0 ? 0 : 280)
    }
    Shortcut {
        sequence: "Ctrl+]"
        onActivated: appState.rightSidebarVisible = !appState.rightSidebarVisible
    }
    // Cmd+L — focus the favorites/files search field (best-effort: also
    // flips sidebar to Favorites mode so the field is visible).
    Shortcut {
        sequence: "Ctrl+L"
        onActivated: { appState.sidebarMode = 0; /* TODO: forwardFocus to search field */ }
    }
    // Cmd+Ctrl+M (Swift) → Ctrl+Shift+M on Windows — toggle mic recording.
    Shortcut {
        sequences: ["Ctrl+Shift+M", "Ctrl+Meta+M"]
        onActivated: { if (typeof voice !== "undefined" && voice) voice.toggleRecording() }
    }
    // Cmd+Shift+K — clear current terminal line (send ANSI EL2 + CR).
    Shortcut {
        sequence: "Ctrl+Shift+K"
        onActivated: {
            if (appState.activeTabId) {
                terminal.write(appState.activeTabId, "[2K\r")
            }
        }
    }

    // ─── Split-pane shortcuts ───────────────────────────────────────────
    // Vertical = side-by-side (Ctrl+\), horizontal = stacked (Ctrl+Shift+\).
    // Close focused pane = Ctrl+Shift+W. Focus left/right between two panes
    // with Ctrl+Alt+← / →. All ops delegate to the active SplitContainer.
    // SPEC-110 — when the active tab has a pane tree, split/close operate on
    // the focused leaf. Otherwise fall back to the legacy 2-pane Split.
    Shortcut {
        sequence: "Ctrl+\\"
        onActivated: {
            if (appState.tabPaneTree(appState.activeTabId).split !== undefined
                  || appState.tabPaneTree(appState.activeTabId).leaf !== undefined) {
                appState.splitPane(appState.activeTabId, term.focusedSessionId, "vertical")
            } else {
                appState.splitActive("vertical")
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+Shift+\\"
        onActivated: {
            if (appState.tabPaneTree(appState.activeTabId).split !== undefined
                  || appState.tabPaneTree(appState.activeTabId).leaf !== undefined) {
                appState.splitPane(appState.activeTabId, term.focusedSessionId, "horizontal")
            } else {
                appState.splitActive("horizontal")
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+Shift+W"
        onActivated: {
            if (appState.tabPaneTree(appState.activeTabId).split !== undefined) {
                appState.closePaneInTree(appState.activeTabId, term.focusedSessionId)
            } else {
                const sc = term.splitContainer
                if (sc && sc.isSplit) sc.closePane(sc.focusedSide)
            }
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
