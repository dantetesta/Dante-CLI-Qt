// Bare-bones terminal view — TextArea capturing keystrokes and showing PTY
// output. NOT a true terminal emulator (no VT100 state machine yet), but
// good enough for shells like cmd/pwsh in REPL mode. Replace with a real
// vt-aware widget (libvterm + QQuickPaintedItem) as a follow-up.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink

    function cycleTab(delta) {
        // TODO: implement via tabs model
    }

    Component.onCompleted: {
        // Spawn the shell for the active tab on first paint.
        const id = appState.activeTabId
        if (id) terminal.spawn(id, "")
    }

    Connections {
        target: appState
        function onActiveTabIdChanged() {
            const id = appState.activeTabId
            if (id) terminal.spawn(id, "")
        }
    }

    Connections {
        target: terminal
        function onOutputReceived(sessionId, chunk) {
            // Find the current tab; for now we have a single visible terminal.
            if (sessionId === appState.activeTabId) {
                output.append(chunk)
            }
        }
        function onCwdChanged(sessionId, cwd) {
            if (sessionId === appState.activeTabId) header.cwdText = cwd
        }
        function onSessionExited(sessionId, code) {
            if (sessionId === appState.activeTabId) {
                output.append("\n[shell exited " + code + "]\n")
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: header
            property string cwdText: "~"
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.surfaceLow
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.borderSoft
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10
                Text { text: ">_"; color: Theme.fgMuted; font.family: Theme.fontMono; font.pixelSize: 11 }
                Text { text: header.cwdText; color: Theme.fgMuted; font.family: Theme.fontMono; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.fillWidth: true }
                Text { text: "● ativo"; color: Theme.success; font.pixelSize: 11 }
            }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            background: Rectangle { color: Theme.ink }

            TextArea {
                id: output
                readOnly: false
                wrapMode: TextArea.WrapAnywhere
                color: Theme.fg
                font.family: Theme.fontMono
                font.pixelSize: appState.fontSize > 0 ? appState.fontSize : 13
                selectByMouse: true
                background: null
                placeholderText: "Iniciando shell…"

                Keys.onReturnPressed: {
                    // Submit current line back to PTY (everything after the last newline).
                    const id = appState.activeTabId
                    const lastNl = output.text.lastIndexOf("\n")
                    const line = lastNl < 0 ? output.text : output.text.substring(lastNl + 1)
                    terminal.write(id, line + "\r")
                    output.append("\n")
                }
                Keys.onPressed: (event) => {
                    // Forward Ctrl+C / Ctrl+D as control codes.
                    if (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_C) {
                        terminal.write(appState.activeTabId, "")
                        event.accepted = true
                    }
                    if (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_D) {
                        terminal.write(appState.activeTabId, "")
                        event.accepted = true
                    }
                }
            }
        }
    }
}
