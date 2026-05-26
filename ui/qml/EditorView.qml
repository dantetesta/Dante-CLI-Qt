// SPEC-021 — minimal editor pane. Mirrors the Swift `EditorPaneView`:
// monospaced TextArea with line numbers, dirty-flag indicator in the header,
// Cmd+S to save. Syntax highlight lands in a follow-up (KSyntaxHighlighting
// or a C++ QSyntaxHighlighter) — for now the buffer renders monochrome but
// fully editable.
//
// AppState owns the buffer (so undo/redo can later be wired through
// QUndoStack at the controller level instead of the QML TextArea).
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    /// Re-evaluated whenever AppState fires tabsChanged or active tab flips.
    /// We watch _bump (incremented by Connections below) so the bindings
    /// don't go stale when the user toggles dirty / saves / renames.
    property int _bump: 0

    readonly property string activeId: appState.activeTabId
    readonly property string path:     (_bump, appState.editorPath(activeId))
    readonly property string lang:     (_bump, appState.editorLanguage(activeId))
    readonly property bool   dirty:    (_bump, appState.editorDirty(activeId))

    Connections {
        target: appState
        ignoreUnknownSignals: true
        function onTabsChanged()        { root._bump += 1 }
        function onActiveTabIdChanged() { root._bump += 1 }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header — filename + dirty dot + language chip + Save button.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: Theme.surfaceLow
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1; color: Theme.borderSoft
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 8
                spacing: 10

                Text {
                    text: "📝"
                    font.pixelSize: 13
                }
                Text {
                    Layout.fillWidth: true
                    text: root.path && root.path.length > 0
                          ? root.path.split("/").pop() + (root.dirty ? "  ●" : "")
                          : qsTr("Sem arquivo")
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    elide: Text.ElideMiddle
                    ToolTip.text: root.path
                    ToolTip.visible: pathHover.hovered && root.path.length > 0
                    ToolTip.delay: 400
                    HoverHandler { id: pathHover }
                }
                // Language chip.
                Rectangle {
                    visible: root.lang.length > 0
                    radius: 4
                    color: Theme.surfaceTop
                    border.color: Theme.borderSoft
                    implicitWidth: langText.implicitWidth + 12
                    implicitHeight: 20
                    Text {
                        id: langText
                        anchors.centerIn: parent
                        text: root.lang.toUpperCase()
                        color: Theme.fgMuted
                        font.family: Theme.fontMono
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                    }
                }
                Button {
                    text: qsTr("Salvar")
                    enabled: root.dirty && root.path.length > 0
                    onClicked: appState.saveEditor(root.activeId)
                    background: Rectangle {
                        color: parent.enabled
                            ? (parent.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
                            : Theme.surfaceTop
                        radius: 4
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "white" : Theme.fgDim
                        font.family: Theme.fontSans
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 10; rightPadding: 10
                    }
                }
            }
        }

        // Body — line numbers gutter + scrollable TextArea.
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Line numbers column.
            Rectangle {
                Layout.preferredWidth: 56
                Layout.fillHeight: true
                color: Theme.surfaceLow
                Rectangle {
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 1
                    color: Theme.borderSoft
                }
                Text {
                    id: lineNumbers
                    anchors.fill: parent
                    anchors.topMargin: 8
                    anchors.rightMargin: 8
                    color: Theme.fgFaint
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    text: {
                        const lines = textArea.lineCount
                        let s = ""
                        for (let i = 1; i <= lines; ++i) s += i + "\n"
                        return s
                    }
                }
            }

            // TextArea inside ScrollView so vertical scrolling works for long
            // files. The TextArea autosizes height; we bind onTextChanged →
            // appState.setEditorContent so the dirty flag flips immediately.
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: textArea
                    text: (root._bump, appState.editorContent(root.activeId))
                    color: Theme.fg
                    font.family: Theme.fontMono
                    font.pixelSize: 13
                    selectByMouse: true
                    wrapMode: TextArea.NoWrap
                    background: Rectangle { color: Theme.ink }
                    onTextChanged: {
                        if (root.activeId)
                            appState.setEditorContent(root.activeId, text)
                    }
                    Keys.onPressed: (event) => {
                        // Cmd+S / Ctrl+S — save. The global shortcut works
                        // too, but having it here makes the editor work when
                        // it's nested inside a popup later.
                        if (event.modifiers & Qt.ControlModifier
                                && event.key === Qt.Key_S) {
                            appState.saveEditor(root.activeId)
                            event.accepted = true
                        }
                    }
                }
            }
        }
    }
}
