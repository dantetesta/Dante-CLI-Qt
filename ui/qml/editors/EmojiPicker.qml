// EmojiPicker — small popup with a grid of common emojis.
// Mirrors the Swift `EmojiPickerView` (sibling in TestaTerminal). Emits
// `selected(emoji)` on click. Caller decides whether to close after pick.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import ".."

Popup {
    id: root

    // 20 common emojis covering dev/devops/security/misc. Matches
    // AppSettings.recentEmojis default + a handful of staples.
    readonly property var emojis: [
        "💻","🚀","🐳","🦀","🐍","📦","⚡","🔥","✨","🎯",
        "📁","⭐","🔧","🎨","📝","🌐","🔐","🔑","🗄️","💾"
    ]

    signal selected(string emoji)

    modal: true
    focus: true
    padding: 8
    width: 280
    height: 200

    background: Rectangle {
        color: Theme.surfaceTop
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusMd
    }

    contentItem: GridLayout {
        columns: 5
        rowSpacing: 4
        columnSpacing: 4

        Repeater {
            model: root.emojis
            Rectangle {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 36
                radius: Theme.radiusSm
                color: hov.containsMouse ? Theme.surfaceHigh : "transparent"
                border.color: hov.containsMouse ? Theme.accentSoft : "transparent"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 20
                }
                MouseArea {
                    id: hov
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.selected(modelData)
                        root.close()
                    }
                }
            }
        }
    }
}
