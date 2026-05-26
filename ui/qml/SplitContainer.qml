// Hosts 1 or 2 PaneViews for a single tab, with an optional draggable
// divider. Bounded to 2 panes max (matches Swift sibling's common case;
// recursive nesting is out of scope for the overnight delivery).
//
// Reads layout from `appState.tabSplitMode(tabId)`. Listens to the
// AppState.tabSplitChanged signal so toggling a split re-flows live
// without remounting the surrounding Terminal.qml.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: root

    // ──── Inputs ─────────────────────────────────────────────────────────
    property string tabId: ""

    // ──── Reactive view of the split state ───────────────────────────────
    // Recomputed via the small bump counter — appState.tabSplitChanged
    // can't be expressed as a binding-source directly, so we toggle a
    // counter on the signal and depend on it in these properties.
    property int    _bump: 0
    readonly property string splitMode:
        (root._bump, appState.tabSplitMode(root.tabId))
    readonly property string primarySid:
        (root._bump, appState.tabPrimarySessionId(root.tabId))
    readonly property string secondSid:
        (root._bump, appState.tabSecondSessionId(root.tabId))
    readonly property bool   isSplit: splitMode === "vertical" || splitMode === "horizontal"
    readonly property bool   isVertical: splitMode === "vertical"

    // Focus is local UI state — which pane the user clicked last.
    property string focusedSide: "a"      // "a" | "b"

    // Drag-resizable split fraction (a's share of the available extent).
    // Starts at 0.5 every time the split is created.
    property real splitFraction: 0.5

    Connections {
        target: appState
        function onTabSplitChanged(changedId) {
            if (changedId !== root.tabId) return
            root._bump += 1
            // Reset to balanced split on every layout change.
            root.splitFraction = 0.5
            // If the previously-focused side disappeared, snap back to "a".
            if (!root.isSplit) root.focusedSide = "a"
        }
    }

    // ──── Imperative helpers (called by BottomToolbar + Main shortcuts) ──
    function splitVertical()   { appState.splitActive("vertical")   }
    function splitHorizontal() { appState.splitActive("horizontal") }
    function unsplit() {
        // Kill the second session's PTY before AppState forgets the id.
        const sb = root.secondSid
        if (sb) terminal.kill(sb)
        appState.splitActive("")
    }
    function closePane(side) {
        if (!root.isSplit) return
        if (side === "a") {
            // Kill old primary; appState will promote b → a.
            if (root.primarySid) terminal.kill(root.primarySid)
        } else {
            if (root.secondSid) terminal.kill(root.secondSid)
        }
        appState.closeActivePane(side)
        root.focusedSide = "a"
    }
    function focusSide(side) {
        if (!root.isSplit && side === "b") return
        root.focusedSide = side
        if (side === "a" && paneA.item) paneA.item.view.forceActiveFocus()
        if (side === "b" && paneB.item && paneB.item.view) paneB.item.view.forceActiveFocus()
    }

    // ──── Layout ─────────────────────────────────────────────────────────
    // Two Loaders so we can use a single layout-aware container for both
    // single and split modes; the divider Loader sits between them.

    // PaneA — explicit geometry in both modes (no anchors). In single
    // mode it fills the container; in split mode it occupies the "a"
    // region computed from splitFraction. Avoiding anchors entirely
    // sidesteps the "anchors override x/y" precedence trap.
    Loader {
        id: paneA
        x: 0
        y: 0
        width:  root.isSplit && root.isVertical
                    ? Math.max(80, root.width * root.splitFraction - 2)
                    : root.width
        height: root.isSplit && !root.isVertical
                    ? Math.max(80, root.height * root.splitFraction - 2)
                    : root.height
        active: true
        sourceComponent: paneComponent
        onLoaded: {
            item.tabId      = root.tabId
            item.side       = "a"
            item.sessionId  = root.primarySid
            item.closable   = root.isSplit
            item.focused    = root.focusedSide === "a"
            item.clicked.connect(function(){ root.focusSide("a") })
            item.closeRequested.connect(function(){ root.closePane("a") })
        }
    }

    Component {
        id: paneComponent
        PaneView {}
    }

    Binding { target: paneA.item; property: "closable";  value: root.isSplit;             when: paneA.item }
    Binding { target: paneA.item; property: "focused";   value: root.focusedSide === "a"; when: paneA.item }
    Binding { target: paneA.item; property: "sessionId"; value: root.primarySid;          when: paneA.item }

    // Divider — Loader so it only exists in split mode.
    Loader {
        id: divider
        active: root.isSplit
        x: root.isVertical ? Math.max(80, root.width * root.splitFraction - 2) : 0
        y: root.isVertical ? 0 : Math.max(80, root.height * root.splitFraction - 2)
        width:  root.isVertical ? 4 : root.width
        height: root.isVertical ? root.height : 4

        sourceComponent: Rectangle {
            id: bar
            color: dragMa.pressed
                       ? Theme.accent
                       : (dragMa.containsMouse ? Theme.borderStrong : Theme.borderSoft)
            Behavior on color { ColorAnimation { duration: 100 } }

            MouseArea {
                id: dragMa
                anchors.fill: parent
                anchors.leftMargin:  root.isVertical ? -3 : 0
                anchors.rightMargin: root.isVertical ? -3 : 0
                anchors.topMargin:    root.isVertical ? 0  : -3
                anchors.bottomMargin: root.isVertical ? 0  : -3
                hoverEnabled: true
                cursorShape: root.isVertical ? Qt.SplitHCursor : Qt.SplitVCursor
                property real anchorFraction: 0.5
                property real anchorPos: 0
                onPressed: (mouse) => {
                    anchorFraction = root.splitFraction
                    anchorPos = root.isVertical
                        ? mouse.x + bar.x
                        : mouse.y + bar.y
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    const cur = root.isVertical ? mouse.x + bar.x : mouse.y + bar.y
                    const total = root.isVertical ? root.width : root.height
                    if (total <= 0) return
                    const delta = (cur - anchorPos) / total
                    root.splitFraction = Math.max(0.1, Math.min(0.9, anchorFraction + delta))
                }
            }
        }
    }

    // Pane B — only mounted in split mode.
    Loader {
        id: paneB
        active: root.isSplit
        x: root.isVertical ? Math.max(80, root.width * root.splitFraction + 2) : 0
        y: root.isVertical ? 0 : Math.max(80, root.height * root.splitFraction + 2)
        width:  root.isVertical ? Math.max(80, root.width  - (root.width  * root.splitFraction + 2)) : root.width
        height: root.isVertical ? root.height : Math.max(80, root.height - (root.height * root.splitFraction + 2))
        sourceComponent: paneComponent
        onLoaded: {
            item.tabId      = root.tabId
            item.side       = "b"
            item.sessionId  = root.secondSid
            item.closable   = true
            item.focused    = root.focusedSide === "b"
            item.clicked.connect(function(){ root.focusSide("b") })
            item.closeRequested.connect(function(){ root.closePane("b") })
        }
    }
    Binding { target: paneB.item; property: "focused";   value: root.focusedSide === "b"; when: paneB.item }
    Binding { target: paneB.item; property: "sessionId"; value: root.secondSid;           when: paneB.item }
}
