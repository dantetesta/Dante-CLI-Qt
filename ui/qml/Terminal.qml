// Active-tab terminal surface. Delegates the actual layout (single pane
// vs. 2-pane split) to SplitContainer, which keys itself off the active
// tab id and reads splitMode/sessionIds reactively from AppState.
//
// Re-mounting the SplitContainer on activeTabId change is intentional —
// it's the simplest way to ensure each tab keeps its own focusedSide /
// splitFraction local UI state without us threading per-tab caches.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    // Expose the active SplitContainer so Main.qml shortcuts can drive it.
    property alias splitContainer: container

    function cycleTab(delta) {
        // TODO: implement via tabs model.
    }

    SplitContainer {
        id: container
        anchors.fill: parent
        anchors.margins: 0
        tabId: appState.activeTabId
    }
}
