// Active-tab content surface. Picks the right engine per tab.kind:
//   • TabKind::Editor (1)        → EditorView (SPEC-021)
//   • appState.tabGridCols > 0   → multi-pane GridWorkspace
//   • otherwise                  → SplitContainer (single pane / 2-pane)
//
// Re-evaluated whenever AppState fires tabSplitChanged(activeId) or the
// active tab id changes, so the user sees the new layout immediately on
// Aplicar / Sair / abrir um arquivo via ⌘O.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    // Expose the active SplitContainer so Main.qml shortcuts can drive it.
    // (null when the tab is in grid or editor mode.)
    property var splitContainer: splitLoader.item

    function cycleTab(delta) {
        // TODO: implement via tabs model.
    }

    property int _bump: 0
    readonly property int  kind: (_bump, appState.tabKind(appState.activeTabId))
    readonly property bool useEditor: kind === 1   // TabKind::Editor
    readonly property bool useGrid: !useEditor
        && (_bump, appState.tabGridCols(appState.activeTabId) > 0
                  && appState.tabGridRows(appState.activeTabId) > 0)
    readonly property bool useSplit: !useEditor && !useGrid

    Connections {
        target: appState
        ignoreUnknownSignals: true
        function onTabSplitChanged(changedId) {
            if (changedId === appState.activeTabId) root._bump += 1
        }
        function onActiveTabIdChanged() { root._bump += 1 }
        function onTabsChanged()        { root._bump += 1 }
    }

    // ─── Single / 2-pane split (terminal mode) ───
    Loader {
        id: splitLoader
        anchors.fill: parent
        active: root.useSplit
        sourceComponent: SplitContainer {
            tabId: appState.activeTabId
        }
    }

    // ─── N × M grid workspace ───
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

    // ─── Editor pane (SPEC-021) ───
    Loader {
        id: editorLoader
        anchors.fill: parent
        active: root.useEditor
        sourceComponent: EditorView {}
    }
}
