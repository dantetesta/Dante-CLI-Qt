// Single chat bubble. Used inside AIOverlay's ListView.
//
// Props:
//   role     — "user" | "assistant" | "system"
//   content  — markdown-ish string (we render \n as line breaks and inline
//              `code spans` in monospace; full markdown is left for later)
//   timestamp — ISO-8601 string (currently unused visually)
//
// User messages right-align with the accent background; assistant/system
// messages left-align with the surface background. Inline code spans are
// detected via a simple regex on backticks and rendered with rich text.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: root
    property string role: "assistant"
    property string content: ""
    property string timestamp: ""

    readonly property bool isUser:      role === "user"
    readonly property bool isAssistant: role === "assistant"
    readonly property bool isSystem:    role === "system"

    implicitWidth: parent ? parent.width : 380
    implicitHeight: layout.implicitHeight + 8

    RowLayout {
        id: layout
        anchors.left:  parent.left
        anchors.right: parent.right
        anchors.leftMargin:  12
        anchors.rightMargin: 12
        spacing: 0

        // Push to the right for user, left for everyone else.
        Item { Layout.fillWidth: true; visible: root.isUser; Layout.minimumWidth: 30 }

        Rectangle {
            id: bubble
            Layout.maximumWidth: Math.max(160, (root.parent ? root.parent.width : 380) * 0.82)
            Layout.preferredWidth: bubbleCol.implicitWidth + 24
            Layout.alignment: root.isUser ? Qt.AlignRight : Qt.AlignLeft
            color: root.isUser
                   ? Theme.accentSoft
                   : (root.isSystem ? Theme.surfaceLow : Theme.surfaceHigh)
            border.color: root.isUser ? Qt.lighter(Theme.accentSoft, 1.2) : Theme.borderSoft
            border.width: 1
            radius: 10
            implicitHeight: bubbleCol.implicitHeight + 14

            ColumnLayout {
                id: bubbleCol
                anchors.fill: parent
                anchors.margins: 9
                spacing: 4

                Text {
                    visible: root.isSystem
                    text: "system"
                    font.family: Theme.fontMono
                    font.pixelSize: 10
                    color: Theme.fgFaint
                }

                TextEdit {
                    id: body
                    Layout.fillWidth: true
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.RichText
                    color: root.isUser ? Theme.fgStrong : Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    text: root._renderRich(root.content)
                }
            }
        }

        Item { Layout.fillWidth: true; visible: !root.isUser; Layout.minimumWidth: 30 }
    }

    /// Minimal markdown→rich-text. We escape HTML, then map `\n` to <br/>
    /// and `…` inline code spans to <span style="…"> chunks. Triple-backtick
    /// code fences become preformatted <pre> blocks.
    function _renderRich(src) {
        if (!src) return ""
        var s = src.toString()
        // HTML escape first
        s = s.replace(/&/g, "&amp;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;")
             .replace(/"/g, "&quot;")
        // Triple-backtick fenced code
        s = s.replace(/```([\s\S]*?)```/g, function(_, code) {
            return '<pre style="background-color:#121216; border:1px solid #2E2E38; '
                 + 'border-radius:6px; padding:8px; font-family:JetBrains Mono, Consolas, monospace; '
                 + 'font-size:12px; color:#E6E6EB;">' + code.replace(/^\n+|\n+$/g,"") + '</pre>'
        })
        // Inline code
        s = s.replace(/`([^`\n]+)`/g, function(_, code) {
            return '<span style="background-color:#202026; border:1px solid #2E2E38; '
                 + 'border-radius:4px; padding:1px 4px; font-family:JetBrains Mono, Consolas, monospace; '
                 + 'font-size:12px;">' + code + '</span>'
        })
        // Bold **x**
        s = s.replace(/\*\*([^*\n]+)\*\*/g, "<b>$1</b>")
        // Line breaks
        s = s.replace(/\n/g, "<br/>")
        return s
    }
}
