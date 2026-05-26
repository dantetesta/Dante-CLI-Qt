// One tab pill. SPEC-120 brought it to feature parity with the Swift chip:
//   • Right-click → context menu (Renomear, Cor, Emoji, Tema, Fixar /
//     Desafixar, Duplicar, Fechar)
//   • Double-click on title → inline rename (TextField; Enter commits, Esc
//     cancels)
//   • Pin/Unpin — pinned tabs get an orange 📌 prefix and `closeTab()` skips
//     them at the AppState level
//   • Quick color palette + emoji shortcut inside the menu
import QtQuick 6.5
import QtQuick.Controls 6.5
import "."

Rectangle {
    id: chip
    property string tabId
    property string title:  ""
    property color  tint:   "#0A84FF"
    property string emoji:  ""
    property bool   pinned: false
    property bool   isActive: false

    signal select()
    signal close()

    width:  Math.min(220, content.implicitWidth + 56)
    height: 36
    radius: 6

    color: Qt.rgba(chip.tint.r, chip.tint.g, chip.tint.b,
                   isActive ? 0.28 : (mouseArea.containsMouse ? 0.16 : 0.10))
    border.color: Qt.rgba(chip.tint.r, chip.tint.g, chip.tint.b, isActive ? 0.65 : 0.20)
    border.width: isActive ? 2 : 1

    scale: mouseArea.pressed ? 0.97 : 1.0
    transformOrigin: Item.Center

    Behavior on color        { ColorAnimation  { duration: 160; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation  { duration: 160; easing.type: Easing.OutCubic } }
    Behavior on border.width { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    Behavior on scale        { NumberAnimation { duration: 110; easing.type: Easing.OutQuad  } }

    /// True while the inline rename TextField is visible.
    property bool _editing: false

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: (mouse) => {
            if (chip._editing) return
            if (mouse.button === Qt.MiddleButton) chip.close()
            else if (mouse.button === Qt.RightButton) ctxMenu.popup()
            else chip.select()
        }
        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) chip._editing = true
        }
    }

    Row {
        id: content
        anchors.left:   parent.left
        anchors.right:  closeBtn.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 6
        spacing: 6

        Text {
            text: chip.pinned ? "📌" : (chip.emoji.length > 0 ? chip.emoji : "💻")
            font.pixelSize: 13
        }

        // Title — switches between Text and editable TextField. Two siblings
        // (visibility-toggled) instead of a single Loader because keeping the
        // Text alive avoids a re-measurement glitch when the chip animates.
        Item {
            width: Math.min(150, Math.max(titleText.implicitWidth, titleEdit.implicitWidth))
            height: 20

            Text {
                id: titleText
                anchors.fill: parent
                visible: !chip._editing
                text: chip.title
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 13
                font.weight: chip.isActive ? Font.Bold : Font.Normal
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }

            TextField {
                id: titleEdit
                anchors.fill: parent
                visible: chip._editing
                text: chip.title
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 13
                selectByMouse: true
                background: Rectangle {
                    color: Theme.surfaceLow
                    border.color: Theme.accent
                    radius: 3
                }
                onActiveFocusChanged: if (!activeFocus && chip._editing) commit(false)
                Keys.onReturnPressed:  commit(true)
                Keys.onEscapePressed:  { chip._editing = false }
                onVisibleChanged: if (visible) { selectAll(); forceActiveFocus() }
                function commit(write) {
                    if (write && text.trim().length > 0)
                        appState.setTabTitle(chip.tabId, text)
                    chip._editing = false
                }
            }
        }
    }

    Rectangle {
        id: closeBtn
        width: 18; height: 18
        radius: 9
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        color: closeArea.containsMouse ? Theme.danger : "transparent"
        visible: (chip.isActive || mouseArea.containsMouse) && !chip.pinned
        opacity: visible ? 1.0 : 0.0

        Behavior on color   { ColorAnimation  { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Text {
            anchors.centerIn: parent
            text: "×"
            color: closeArea.containsMouse ? "white" : Theme.fgMuted
            font.pixelSize: 13
            font.bold: true
            Behavior on color { ColorAnimation { duration: 140 } }
        }
        MouseArea {
            id: closeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: chip.close()
        }
    }

    /* ─── Right-click context menu (SPEC-120) ─── */
    Menu {
        id: ctxMenu
        // Mirror the Swift sibling's order. Each entry calls the matching
        // AppState Q_INVOKABLE so it persists immediately.
        MenuItem {
            text: qsTr("Renomear")
            onTriggered: chip._editing = true
        }
        MenuItem {
            text: chip.pinned ? qsTr("Desafixar aba") : qsTr("Fixar aba")
            onTriggered: appState.setTabPinned(chip.tabId, !chip.pinned)
        }
        MenuItem {
            text: qsTr("Duplicar aba")
            onTriggered: appState.duplicateTab(chip.tabId)
        }
        MenuSeparator {}

        Menu {
            title: qsTr("Cor")
            Repeater {
                model: [
                    {label: "Azul",     hex: "#0A84FF"},
                    {label: "Vermelho", hex: "#DC2626"},
                    {label: "Verde",    hex: "#2ECC71"},
                    {label: "Amarelo",  hex: "#F59E0B"},
                    {label: "Roxo",     hex: "#A855F7"},
                    {label: "Laranja",  hex: "#FB923C"},
                    {label: "Índigo",   hex: "#7C82FF"},
                    {label: "Rosa",     hex: "#EC4899"}
                ]
                delegate: MenuItem {
                    text: modelData.label
                    icon.source: ""    // intentionally empty so the color square
                                       // shown below is the only color affordance
                    onTriggered: appState.setTabColor(chip.tabId, modelData.hex)
                }
            }
        }
        Menu {
            title: qsTr("Emoji")
            Repeater {
                model: ["💻", "🖥", "🐚", "⚡", "🦀", "🐍", "🟨", "🧠", "📦", "🔧", "🧪", "🚀"]
                delegate: MenuItem {
                    text: modelData
                    onTriggered: appState.setTabEmoji(chip.tabId, modelData)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Sem emoji")
                onTriggered: appState.setTabEmoji(chip.tabId, "")
            }
        }
        Menu {
            title: qsTr("Tema do terminal")
            Repeater {
                model: schemes ? schemes.list() : []
                delegate: MenuItem {
                    text: modelData.name
                    onTriggered: appState.setTabScheme(chip.tabId, modelData.id)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Usar tema global")
                onTriggered: appState.setTabScheme(chip.tabId, "")
            }
        }

        MenuSeparator {}
        MenuItem {
            text: qsTr("Fechar aba")
            enabled: !chip.pinned
            onTriggered: chip.close()
        }
    }
}
