// Single terminal pane: header (cwd + close button) + TerminalView.
// Used by SplitContainer; not meant to be mounted standalone outside a
// SplitContainer (parent wires `focused`, `closable`, etc.).
//
// Mirrors the Swift sibling's PaneView/TerminalNSView pairing — header
// is a thin chrome strip, the rest is the live VT view. Click anywhere
// in the pane (header or body) requests focus from the parent.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import dante.ui 1.0
import "."

Rectangle {
    id: root

    // ──── Inputs ─────────────────────────────────────────────────────────
    property string tabId: ""
    property string sessionId: ""
    property string side: "a"          // "a" | "b" — header label hint only
    property bool   focused: false
    property bool   closable: false    // hide "×" when only one pane exists
    property alias  view: termView     // expose TerminalView for parent forceFocus

    // ──── Outputs ────────────────────────────────────────────────────────
    signal clicked()                   // pane wants focus
    signal closeRequested()            // user hit "×" in header

    color: Theme.ink
    radius: 4
    border.width: root.focused ? 2 : 1
    border.color: root.focused ? Theme.accent : Theme.borderSoft
    Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

    // Click anywhere → request focus. Sits *below* the TerminalView so the
    // terminal still receives clicks for its own selection/cursor handling.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        propagateComposedEvents: true
        onPressed: (mouse) => {
            root.clicked()
            mouse.accepted = false   // let TerminalView consume too
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.focused ? 2 : 1
        spacing: 0

        // ── Pane header (cwd, focus dot, close × when 2 panes) ──────────
        Rectangle {
            id: header
            property string cwdText: "~"
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: Theme.surfaceLow
            radius: 3

            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.borderSoft
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 8

                Text {
                    text: ">_"
                    color: Theme.fgFaint
                    font.family: Theme.fontMono
                    font.pixelSize: 11
                }
                Text {
                    text: header.cwdText
                    color: root.focused ? Theme.fg : Theme.fgMuted
                    font.family: Theme.fontMono
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                Text {
                    visible: root.focused
                    text: "●"
                    color: Theme.accent
                    font.pixelSize: 10
                }
                // Close button — only when this pane is part of a split.
                Rectangle {
                    visible: root.closable
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    radius: 9
                    color: closeMa.containsMouse ? Theme.danger : "transparent"
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: closeMa.containsMouse ? Theme.fgStrong : Theme.fgMuted
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: closeMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeRequested()
                    }
                }
            }
        }

        // ── The actual VT view ──────────────────────────────────────────
        TerminalView {
            id: termView
            Layout.fillWidth: true
            Layout.fillHeight: true
            sessionId: root.sessionId
            fontFamily: Theme.fontMono
            fontPixelSize: appState.fontSize > 0 ? appState.fontSize : 13
            backgroundColor: Theme.ink
            foregroundColor: Theme.fg
            activeFocusOnTab: true

            // Spawn on creation. Idempotent on the C++ side.
            Component.onCompleted: {
                if (root.sessionId) terminal.spawn(root.sessionId, "")
            }
            // Re-spawn when sessionId changes (e.g. promotion after closing
            // the sibling). Cheap when already running.
            onSessionIdChanged: {
                if (root.sessionId) terminal.spawn(root.sessionId, "")
            }

            onWriteRequested: (sid, bytes) => terminal.write(sid, bytes)
            onResizeRequested: (sid, c, r)  => terminal.resize(sid, c, r)
            onOscEvent: (code, payload) => {
                if (code === 7) {
                    var p = payload
                    if (p.indexOf("file://") === 0) {
                        var slash = p.indexOf("/", 7)
                        if (slash >= 0) p = p.substring(slash)
                    }
                    header.cwdText = decodeURIComponent(p)
                }
            }

            // Mouse press inside the VT also bubbles focus request up.
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true
                onPressed: (mouse) => {
                    root.clicked()
                    mouse.accepted = false
                }
            }
        }
    }

    // Output routing — only feed our own session's bytes into our view.
    Connections {
        target: terminal
        function onOutputReceived(sid, chunk) {
            if (sid === root.sessionId) termView.feed(chunk)
        }
        function onSessionExited(sid, code) {
            if (sid === root.sessionId) {
                termView.feed("\r\n[shell exited " + code + "]\r\n")
            }
        }
    }
}
