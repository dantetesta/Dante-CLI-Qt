// FavoriteEditor — modal popup to create/edit a Favorite entry.
// Mirrors the Swift `FavoriteEditorSheet`. Caller passes either an empty
// `existingId` (= create) or a non-empty id (= edit). On Save emits
// `saved(props)` with the assembled JS object; the parent decides whether
// to call `favorites.addFull(props)` or `favorites.update(id, props)`.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import QtQuick.Dialogs 6.5
import ".."

Popup {
    id: root
    modal: true
    focus: true
    padding: 0

    width: 520
    height: 560
    anchors.centerIn: parent

    /// Empty when creating; populated when editing.
    property string existingId: ""
    property string nameText: ""
    property string pathText: ""
    property string emojiText: ""
    property string colorHex: "#7C82FF"
    property string initialCommandText: ""
    property string tagsText: ""

    readonly property var colorPresets: [
        "#7C82FF", "#0A84FF", "#2ECC71", "#F59E0B",
        "#DC2626", "#A855F7", "#EC4899", "#14B8A6"
    ]

    signal saved(var props)
    signal cancelled()

    /// Pre-fills the fields. Called by Sidebar before opening the popup.
    function loadFromMap(m) {
        existingId         = m && m.id        ? m.id        : ""
        nameText           = m && m.name      ? m.name      : ""
        pathText           = m && m.path      ? m.path      : ""
        emojiText          = m && m.emoji     ? m.emoji     : ""
        colorHex           = m && m.colorHex  ? m.colorHex  : "#7C82FF"
        initialCommandText = m && m.initialCommand ? m.initialCommand : ""
        tagsText           = m && m.tags ? (m.tags.join ? m.tags.join(", ") : "") : ""
    }
    function reset() { loadFromMap(null) }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
    }

    onOpened: nameField.forceActiveFocus()

    FileDialog {
        id: folderPicker
        title: "Escolher pasta"
        fileMode: FileDialog.OpenFile
        // Note: Qt6 FileDialog can't pick directories in pure QML;
        // we accept any file inside the target dir and derive the path.
        // TODO: switch to FolderDialog (QtQuick.Dialogs >= 6.3) once
        // the project minimum is bumped — it's available but the
        // current CMakeLists doesn't import QtLabsPlatform.
        onAccepted: {
            const url = selectedFile.toString()
            // strip "file://" prefix; works on macOS + Linux.
            let p = url.replace(/^file:\/\//, "")
            // Drop trailing filename → leave directory.
            const slash = p.lastIndexOf("/")
            if (slash > 0) p = p.substring(0, slash)
            root.pathText = p
            if (root.nameText.length === 0) {
                const segs = p.split("/")
                if (segs.length > 0) root.nameText = segs[segs.length - 1]
            }
        }
    }

    EmojiPicker {
        id: emojiPicker
        onSelected: function(e) { root.emojiText = e }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        Text {
            text: root.existingId.length > 0 ? "Editar Favorito" : "Novo Favorito"
            color: Theme.fgStrong
            font.family: Theme.fontSans
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        // Name
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: "Nome"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            TextField {
                id: nameField
                Layout.fillWidth: true
                text: root.nameText
                onTextChanged: root.nameText = text
                placeholderText: "Ex: Plugin Auth"
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

        // Path
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: "Caminho"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                TextField {
                    id: pathField
                    Layout.fillWidth: true
                    text: root.pathText
                    onTextChanged: root.pathText = text
                    placeholderText: "/path/to/project"
                    color: Theme.fg
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    background: Rectangle {
                        color: Theme.surfaceHigh
                        border.color: pathField.activeFocus ? Theme.accent : Theme.border
                        radius: Theme.radiusSm
                    }
                }
                Button {
                    text: "📁  Browse"
                    onClicked: folderPicker.open()
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : Theme.surfaceHigh
                        border.color: Theme.border
                        radius: Theme.radiusSm
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fg
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // Emoji + Color row
        RowLayout {
            spacing: 14
            Layout.fillWidth: true

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
                        text: root.emojiText.length > 0 ? root.emojiText : "🙂"
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

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Text { text: "Cor"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                RowLayout {
                    spacing: 6
                    Repeater {
                        model: root.colorPresets
                        Rectangle {
                            width: 24; height: 24; radius: 12
                            color: modelData
                            border.width: root.colorHex === modelData ? 2 : 0
                            border.color: Theme.fgStrong
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.colorHex = modelData
                            }
                        }
                    }
                }
            }
        }

        // Initial command
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: "Comando inicial (opcional)"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            TextField {
                id: cmdField
                Layout.fillWidth: true
                text: root.initialCommandText
                onTextChanged: root.initialCommandText = text
                placeholderText: "ex: npm run dev"
                color: Theme.fg
                font.family: Theme.fontMono
                font.pixelSize: 12
                background: Rectangle {
                    color: Theme.surfaceHigh
                    border.color: cmdField.activeFocus ? Theme.accent : Theme.border
                    radius: Theme.radiusSm
                }
            }
        }

        // Tags
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: "Tags (separadas por vírgula)"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            TextField {
                id: tagsField
                Layout.fillWidth: true
                text: root.tagsText
                onTextChanged: root.tagsText = text
                placeholderText: "trabalho, urgente, frontend"
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

        Item { Layout.fillHeight: true }

        // Buttons
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
                enabled: root.nameText.trim().length > 0 && root.pathText.trim().length > 0
                onClicked: {
                    const tags = root.tagsText.split(",")
                        .map(function(s){ return s.trim() })
                        .filter(function(s){ return s.length > 0 })
                    root.saved({
                        id: root.existingId,
                        name: root.nameText.trim(),
                        path: root.pathText.trim(),
                        emoji: root.emojiText,
                        colorHex: root.colorHex,
                        initialCommand: root.initialCommandText,
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
