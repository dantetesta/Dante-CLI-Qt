// Active-tab content surface. Picks the right engine per tab.kind / state:
//   • TabKind::Editor (1)        → EditorView (SPEC-021)
//   • appState.tabPaneTree non-empty → RecursiveSplit (SPEC-110, N panes)
//   • appState.tabGridCols > 0   → multi-pane GridWorkspace
//   • otherwise                  → SplitContainer (single pane / legacy 2-pane)
//
// Re-evaluated whenever AppState fires tabSplitChanged(activeId) or the
// active tab id changes, so the user sees the new layout immediately.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    // Exposed so Main.qml shortcuts can call into the active sub-tree.
    property var splitContainer: splitLoader.item
    property var paneTree: recurseLoader.item

    /// Focused session id — RecursiveSplit propagates this up via property
    /// binding so the global Cmd+Shift+W / split shortcuts know which leaf
    /// to operate on.
    property string focusedSessionId: appState.activeTabId

    function cycleTab(delta) { /* TODO via tabs model */ }

    property int _bump: 0
    readonly property int  kind: (_bump, appState.tabKind(appState.activeTabId))
    readonly property bool useEditor: kind === 1
    readonly property bool useVideo:  kind === 3 // TabKind::Video
    readonly property bool useCalculator: (_bump,
        appState.tabKindString(appState.activeTabId) === "calculator")
    readonly property bool useTree: !useEditor && !useVideo && !useCalculator
        && (_bump, !appState.tabPaneTree(appState.activeTabId).leaf
                  && !appState.tabPaneTree(appState.activeTabId).split
                  ? false : true)
    readonly property bool useGrid: !useEditor && !useVideo && !useTree && !useCalculator
        && (_bump, appState.tabGridCols(appState.activeTabId) > 0
                  && appState.tabGridRows(appState.activeTabId) > 0)
    readonly property bool useSplit: !useEditor && !useVideo && !useTree && !useGrid && !useCalculator

    Connections {
        target: appState
        ignoreUnknownSignals: true
        function onTabSplitChanged(changedId) {
            if (changedId === appState.activeTabId) root._bump += 1
        }
        function onActiveTabIdChanged() {
            root._bump += 1
            // Reset focus to the primary leaf when switching tabs.
            root.focusedSessionId = appState.tabPrimarySessionId(appState.activeTabId)
        }
        function onTabsChanged() { root._bump += 1 }
    }

    Loader {
        id: splitLoader
        anchors.fill: parent
        active: root.useSplit
        sourceComponent: SplitContainer { tabId: appState.activeTabId }
    }

    Loader {
        id: gridLoader
        anchors.fill: parent
        active: root.useGrid
        sourceComponent: GridWorkspace {
            cols:  appState.tabGridCols(appState.activeTabId)
            rows:  appState.tabGridRows(appState.activeTabId)
            spans: appState.tabGridSpans(appState.activeTabId)
        }
    }

    Loader {
        id: editorLoader
        anchors.fill: parent
        active: root.useEditor
        sourceComponent: EditorView {}
    }

    Loader {
        id: calculatorLoader
        anchors.fill: parent
        active: root.useCalculator
        sourceComponent: CalculatorView {}
    }

    Loader {
        id: videoLoader
        anchors.fill: parent
        active: root.useVideo
        sourceComponent: VideoView {}
    }

    // SPEC-110 — recursive pane tree.
    Loader {
        id: recurseLoader
        anchors.fill: parent
        active: root.useTree
        sourceComponent: RecursiveSplit {
            tabId: appState.activeTabId
            node:  appState.tabPaneTree(appState.activeTabId)
            path:  []
            focusedSessionId: root.focusedSessionId
            onFocusedSessionIdChanged: root.focusedSessionId = focusedSessionId
        }
    }
}
