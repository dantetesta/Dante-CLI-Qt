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
        anchors.fill: parent
        spacing: 0

        // ───── Left sidebar ─────
        Sidebar {
            id: sidebar
            Layout.preferredWidth: root.sidebarWidth
            Layout.minimumWidth:   220
            Layout.maximumWidth:   560
            Layout.fillHeight:     true
        }

        // ───── Splitter ─────
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: Theme.borderSoft
            MouseArea {
                anchors.fill: parent
                anchors.leftMargin: -3; anchors.rightMargin: -3
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

    // Global ⌘T / Ctrl+T → new tab.
    Shortcut { sequence: StandardKey.AddTab;   onActivated: appState.addTab("Terminal") }
    Shortcut { sequence: StandardKey.Close;    onActivated: appState.closeTab(appState.activeTabId) }
    Shortcut { sequence: "Ctrl+Shift+]";       onActivated: term.cycleTab(1) }
    Shortcut { sequence: "Ctrl+Shift+[";       onActivated: term.cycleTab(-1) }
}
