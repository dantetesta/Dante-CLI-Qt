// Active-tab terminal surface. Picks one of two engines per tab:
//   • appState.tabGridCols(id) > 0  → multi-pane GridWorkspace (cols × rows
//     with optional spans). Used when the user designed something in the
//     LayoutDesigner that isn't a plain 2-pane split.
//   • otherwise                     → SplitContainer (single pane or 2-pane).
//
// Both views are rebuilt whenever AppState fires tabSplitChanged(activeId),
// so the user sees the new layout immediately on Aplicar / Sair.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    // Expose the active SplitContainer so Main.qml shortcuts can drive it.
    // (null when the tab is in grid mode.)
    property var splitContainer: splitLoader.item

    function cycleTab(delta) {
        // TODO: implement via tabs model.
    }

    // Read on first paint AND whenever the active-tab id or its split state
    // changes. Tracking _bump in the binding forces re-evaluation on signal.
    property int _bump: 0
    readonly property bool useGrid: (_bump, appState.tabGridCols(appState.activeTabId) > 0
                                            && appState.tabGridRows(appState.activeTabId) > 0)

    Connections {
        target: appState
        ignoreUnknownSignals: true
        function onTabSplitChanged(changedId) {
            if (changedId === appState.activeTabId) root._bump += 1
        }
        function onActiveTabIdChanged() { root._bump += 1 }
    }

    // ─── Single / 2-pane split ───
    Loader {
        id: splitLoader
        anchors.fill: parent
        active: !root.useGrid
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
}
