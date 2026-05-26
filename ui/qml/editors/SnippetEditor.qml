// SnippetEditor — modal popup to create/edit a Snippet entry.
// Mirrors `SnippetsSidebar` editor sheet in the Swift sibling.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import ".."
// MovablePopup lives one directory up.

MovablePopup {
    id: root
    width: 560
    height: 520
    minWidth: 420
    minHeight: 360
    title: existingId.length > 0 ? qsTr("Editar Snippet") : qsTr("Novo Snippet")
    icon: "⚡"

    property string existingId: ""
    property string nameText: ""
    property string commandText: ""
    property string emojiText: ""
    property string tagsText: ""

    signal saved(var props)
    signal cancelled()

    function loadFromMap(m) {
        existingId  = m && m.id      ? m.id      : ""
        nameText    = m && m.name    ? m.name    : ""
        commandText = m && m.command ? m.command : ""
        emojiText   = m && m.emoji   ? m.emoji   : ""
        tagsText    = m && m.tags    ? (m.tags.join ? m.tags.join(", ") : "") : ""
    }
    function reset() { loadFromMap(null) }

    onOpened: nameField.forceActiveFocus()

    EmojiPicker {
        id: emojiPicker
        onSelected: function(e) { root.emojiText = e }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Text { text: "Nome"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    text: root.nameText
                    onTextChanged: root.nameText = text
                    placeholderText: "Ex: Deploy production"
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    background: Rectangle {
                        color: Theme.surfaceHigh
                        border.color: nameField.activeFocus ? Theme.accent : Theme.border
                        radius: Theme.radiusSm
                    }
                }
            }
            ColumnLayout {
                spacing: 4
                Text { text: "Emoji"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                Rectangle {
                    width: 64; height: 36
                    radius: Theme.radiusSm
                    color: emojiArea.containsMouse ? Theme.surfaceTop : Theme.surfaceHigh
                    border.color: Theme.border
                    Text {
                        anchors.centerIn: parent
                        text: root.emojiText.length > 0 ? root.emojiText : "⚡"
                        font.pixelSize: 20
                        opacity: root.emojiText.length > 0 ? 1.0 : 0.4
                    }
                    MouseArea {
                        id: emojiArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: emojiPicker.open()
                    }
                }
            }
        }

        // Command — multi-line monospace
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Layout.fillHeight: true
            Text { text: "Comando"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.surfaceHigh
                border.color: cmdArea.activeFocus ? Theme.accent : Theme.border
                radius: Theme.radiusSm

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 6
                    TextArea {
                        id: cmdArea
                        text: root.commandText
                        onTextChanged: root.commandText = text
                        wrapMode: TextEdit.Wrap
                        color: Theme.fg
                        font.family: Theme.fontMono
                        font.pixelSize: 12
                        placeholderText: "git pull && npm install && npm run build"
                        background: null
                        selectByMouse: true
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: "Tags (separadas por vírgula)"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            TextField {
                id: tagsField
                Layout.fillWidth: true
                text: root.tagsText
                onTextChanged: root.tagsText = text
                placeholderText: "deploy, ci"
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 12
                background: Rectangle {
                    color: Theme.surfaceHigh
                    border.color: tagsField.activeFocus ? Theme.accent : Theme.border
                    radius: Theme.radiusSm
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                text: "Cancelar"
                onClicked: { root.cancelled(); root.close() }
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : "transparent"
                    border.color: Theme.border
                    radius: Theme.radiusSm
                }
                contentItem: Text {
                    text: parent.text; color: Theme.fg
                    font.family: Theme.fontSans; font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Button {
                id: saveBtn
                text: root.existingId.length > 0 ? "Salvar" : "Adicionar"
                enabled: root.nameText.trim().length > 0 && root.commandText.trim().length > 0
                onClicked: {
                    const tags = root.tagsText.split(",")
                        .map(function(s){ return s.trim() })
                        .filter(function(s){ return s.length > 0 })
                    root.saved({
                        id: root.existingId,
                        name: root.nameText.trim(),
                        command: root.commandText,
                        emoji: root.emojiText,
                        tags: tags,
                    })
                    root.close()
                }
                background: Rectangle {
                    color: saveBtn.enabled
                        ? (saveBtn.hovered ? Theme.accent : Theme.accentSoft)
                        : Theme.surfaceHigh
                    radius: Theme.radiusSm
                }
                contentItem: Text {
                    text: parent.text
                    color: saveBtn.enabled ? Theme.fgStrong : Theme.fgFaint
                    font.family: Theme.fontSans; font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
