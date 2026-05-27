// SPEC-110 — recursive pane tree renderer. Mirrors the Swift PaneSplitView.
//
// Given a `node` (QVariantMap with either {leaf: sessionId} or
// {split, ratio, first, second}), it either:
//   • renders a single PaneView for the leaf, or
//   • renders two child RecursiveSplits separated by a draggable seam.
//
// The recursion works because QML lets a component self-reference via
// `sourceComponent: Qt.createComponent("RecursiveSplit.qml")` when the
// outer component is the same file. We bind the seam's MouseArea drag to
// `appState.setPaneRatio(tabId, path, newRatio)` so the tree mutation lives
// in AppState (single source of truth + persistence + signal fanout).
import QtQuick 6.5
import QtQuick.Controls 6.5
import "."

Item {
    id: root

    /// The full pane tree node — { leaf } or { split, ratio, first, second }.
    property var    node: ({})
    /// Path from the root pane tree to this node (array of 0/1).
    property var    path: []
    /// Tab id — used by AppState.setPaneRatio to find the right tree.
    property string tabId: ""
    /// True when this subtree contains the currently focused leaf.
    /// Propagates up so the parent can highlight the focused branch.
    property string focusedSessionId: ""

    /// Convenience accessors.
    readonly property bool isLeaf:    node && node.leaf !== undefined
    readonly property bool isSplit:   node && node.split !== undefined
    readonly property bool isVertical: isSplit && node.split === "vertical"
    readonly property real ratio:     isSplit ? (node.ratio || 0.5) : 0.5
    readonly property string leafSid: isLeaf ? node.leaf : ""

    // Leaf — render a PaneView bound to its session.
    Loader {
        anchors.fill: parent
        active: root.isLeaf
        sourceComponent: PaneView {
            tabId: root.tabId
            sessionId: root.leafSid
            focused: root.focusedSessionId === root.leafSid
            closable: true
            onClicked: root.focusedSessionId = root.leafSid
            onCloseRequested: {
                terminal.kill(root.leafSid)
                appState.closePaneInTree(root.tabId, root.leafSid)
            }
        }
    }

    // Split — first child / seam / second child.
    Loader {
        id: firstLoader
        anchors.left:   root.left
        anchors.top:    root.top
        anchors.bottom: root.isVertical ? root.bottom : undefined
        anchors.right:  root.isVertical ? undefined   : root.right
        width:  root.isVertical ? Math.max(80, root.width  * root.ratio - 2) : root.width
        height: root.isVertical ? root.height : Math.max(80, root.height * root.ratio - 2)
        active: root.isSplit
        // Use `source` (URL) instead of `sourceComponent: RecursiveSplit{}`
        // because QML disallows direct recursive instantiation of the same
        // type. Loader-via-URL breaks the dependency cycle. Properties are
        // assigned via onLoaded since we can't bind through the Loader.
        source: root.isSplit ? "RecursiveSplit.qml" : ""
        onLoaded: {
            if (!item) return
            item.tabId = root.tabId
            item.path  = root.path.concat([0])
            item.node  = root.node && root.node.first ? root.node.first : ({})
            item.focusedSessionId = root.focusedSessionId
            item.focusedSessionIdChanged.connect(function() {
                root.focusedSessionId = item.focusedSessionId
            })
        }
        Connections {
            target: root
            ignoreUnknownSignals: true
            function onNodeChanged() {
                if (firstLoader.item)
                    firstLoader.item.node = root.node && root.node.first
                                            ? root.node.first : ({})
            }
            function onFocusedSessionIdChanged() {
                if (firstLoader.item)
                    firstLoader.item.focusedSessionId = root.focusedSessionId
            }
        }
    }

    Loader {
        id: seamLoader
        active: root.isSplit
        x: root.isVertical ? Math.max(80, root.width * root.ratio - 2) : 0
        y: root.isVertical ? 0 : Math.max(80, root.height * root.ratio - 2)
        width:  root.isVertical ? 4 : root.width
        height: root.isVertical ? root.height : 4

        sourceComponent: Rectangle {
            id: seam
            color: seamMa.pressed
                       ? Theme.accent
                       : (seamMa.containsMouse ? Theme.borderStrong : Theme.borderSoft)
            Behavior on color { ColorAnimation { duration: 100 } }
            MouseArea {
                id: seamMa
                anchors.fill: parent
                anchors.leftMargin:   root.isVertical ? -3 : 0
                anchors.rightMargin:  root.isVertical ? -3 : 0
                anchors.topMargin:    root.isVertical ? 0  : -3
                anchors.bottomMargin: root.isVertical ? 0  : -3
                hoverEnabled: true
                cursorShape: root.isVertical ? Qt.SplitHCursor : Qt.SplitVCursor
                property real anchorRatio: 0.5
                property real anchorPos: 0
                onPressed: (mouse) => {
                    anchorRatio = root.ratio
                    anchorPos = root.isVertical
                        ? mouse.x + seam.x
                        : mouse.y + seam.y
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    const cur = root.isVertical ? mouse.x + seam.x : mouse.y + seam.y
                    const total = root.isVertical ? root.width : root.height
                    if (total <= 0) return
                    const delta = (cur - anchorPos) / total
                    const next = Math.max(0.1, Math.min(0.9, anchorRatio + delta))
                    // Mutate the node's ratio inline so the layout reacts at
                    // 60 fps. AppState is updated on release to avoid 60 fps
                    // JSON saves.
                    const cloned = {}
                    for (const k in root.node) cloned[k] = root.node[k]
                    cloned.ratio = next
                    root.node = cloned
                }
                onReleased: {
                    appState.setPaneRatio(root.tabId, root.path, root.ratio)
                }
            }
        }
    }

    Loader {
        anchors.right:  root.right
        anchors.bottom: root.bottom
        anchors.left:   root.isVertical ? undefined : root.left
        anchors.top:    root.isVertical ? root.top : undefined
        width:  root.isVertical ? Math.max(80, root.width  - (root.width  * root.ratio + 2)) : root.width
        height: root.isVertical ? root.height : Math.max(80, root.height - (root.height * root.ratio + 2))
        active: root.isSplit
        source: root.isSplit ? "RecursiveSplit.qml" : ""
        onLoaded: {
            if (!item) return
            item.tabId = root.tabId
            item.path  = root.path.concat([1])
            item.node  = root.node && root.node.second ? root.node.second : ({})
            item.focusedSessionId = root.focusedSessionId
            item.focusedSessionIdChanged.connect(function() {
                root.focusedSessionId = item.focusedSessionId
            })
        }
        Connections {
            target: root
            ignoreUnknownSignals: true
            function onNodeChanged() {
                if (secondLoader.item)
                    secondLoader.item.node = root.node && root.node.second
                                             ? root.node.second : ({})
            }
            function onFocusedSessionIdChanged() {
                if (secondLoader.item)
                    secondLoader.item.focusedSessionId = root.focusedSessionId
            }
        }
        id: secondLoader
    }
}
