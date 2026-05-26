// CredentialEditor — modal popup to create/edit a Credential.
// Mirrors the Swift `CredentialEditorSheet`: name, kind, emoji, notes, and
// a dynamic list of (label, value, masked) fields. Kind dropdown swaps in
// default fields if the user hasn't customized them yet.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import ".."

Popup {
    id: root
    modal: true
    focus: true
    padding: 0

    width: 620
    height: 640
    anchors.centerIn: parent

    // CredentialKind enum: 0=Ssh, 1=Ftp, 2=Api, 3=Database, 4=Custom.
    // Numbering matches `dante::CredentialKind`. Updating here without
    // syncing Models.h will break persistence.
    readonly property var kinds: [
        { idx: 0, label: "SSH",      emoji: "🔐" },
        { idx: 1, label: "FTP",      emoji: "📁" },
        { idx: 2, label: "API",      emoji: "🔑" },
        { idx: 3, label: "Database", emoji: "🗄️" },
        { idx: 4, label: "Custom",   emoji: "📋" },
    ]

    property string existingId: ""
    property string nameText: ""
    property int    kindIdx: 0
    property string emojiText: "🔐"
    property string notesText: ""
    // Each entry is a JS object {label, value, masked}. We keep it as a
    // plain JS array (not a ListModel) because the user mutates fields
    // frequently and rebuilding the model on every keystroke is cheaper
    // than threading binding updates through ListModel APIs.
    property var fields: []
    /// True once the user manually edits the field list — blocks default
    /// reload on kind change (mirror of Swift `customized` flag).
    property bool fieldsCustomized: false

    signal saved(var props)
    signal cancelled()

    function defaultFieldsFor(kindIdx) {
        switch (kindIdx) {
        case 0: return [
            { label: "host",     value: "",   masked: false },
            { label: "port",     value: "22", masked: false },
            { label: "user",     value: "",   masked: false },
            { label: "password", value: "",   masked: true  },
            { label: "key_path", value: "",   masked: false },
        ]
        case 1: return [
            { label: "host",     value: "",   masked: false },
            { label: "port",     value: "21", masked: false },
            { label: "user",     value: "",   masked: false },
            { label: "password", value: "",   masked: true  },
        ]
        case 2: return [
            { label: "url",      value: "",   masked: false },
            { label: "key",      value: "",   masked: true  },
        ]
        case 3: return [
            { label: "host",     value: "",   masked: false },
            { label: "port",     value: "",   masked: false },
            { label: "user",     value: "",   masked: false },
            { label: "password", value: "",   masked: true  },
            { label: "db",       value: "",   masked: false },
        ]
        default: return []
        }
    }

    function loadFromMap(m) {
        existingId = m && m.id        ? m.id        : ""
        nameText   = m && m.name      ? m.name      : ""
        kindIdx    = m && m.kind !== undefined ? m.kind : 0
        emojiText  = m && m.emoji     ? m.emoji     : kinds[kindIdx].emoji
        notesText  = m && m.notes     ? m.notes     : ""
        if (m && m.fields && m.fields.length !== undefined) {
            fields = m.fields.slice()
            fieldsCustomized = true
        } else {
            fields = defaultFieldsFor(kindIdx)
            fieldsCustomized = false
        }
    }
    function reset() {
        existingId = ""
        nameText = ""
        kindIdx = 0
        emojiText = kinds[0].emoji
        notesText = ""
        fields = defaultFieldsFor(0)
        fieldsCustomized = false
    }

    /// Validation: at least one field with non-empty label AND non-empty value.
    function isValid() {
        if (nameText.trim().length === 0) return false
        for (let i = 0; i < fields.length; ++i) {
            const f = fields[i]
            if (f.label && f.label.length > 0 && f.value && f.value.length > 0) {
                return true
            }
        }
        return false
    }

    function addField() {
        const next = fields.slice()
        next.push({ label: "", value: "", masked: false })
        fields = next
        fieldsCustomized = true
    }
    function removeFieldAt(i) {
        const next = fields.slice()
        next.splice(i, 1)
        fields = next
        fieldsCustomized = true
    }
    function patchField(i, patch) {
        const next = fields.slice()
        next[i] = Object.assign({}, next[i], patch)
        fields = next
        fieldsCustomized = true
    }

    onKindIdxChanged: {
        // Only swap in defaults if the user hasn't manually edited the
        // field list — mirrors the Swift sibling's "preserve customized
        // shape" logic.
        if (!fieldsCustomized) {
            fields = defaultFieldsFor(kindIdx)
        }
        if (emojiText === "" || isDefaultEmoji(emojiText)) {
            emojiText = kinds[kindIdx].emoji
        }
    }

    function isDefaultEmoji(e) {
        for (let i = 0; i < kinds.length; ++i) {
            if (kinds[i].emoji === e) return true
        }
        return false
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
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
            text: root.existingId.length > 0 ? "Editar Credencial" : "Nova Credencial"
            color: Theme.fgStrong
            font.family: Theme.fontSans
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        // Name + Kind + Emoji
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
                    placeholderText: "Ex: VPS Produção"
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
                Layout.preferredWidth: 160
                Text { text: "Tipo"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                ComboBox {
                    id: kindBox
                    Layout.fillWidth: true
                    model: root.kinds
                    textRole: "label"
                    currentIndex: root.kindIdx
                    onCurrentIndexChanged: root.kindIdx = currentIndex
                    font.pixelSize: 13
                }
            }

            ColumnLayout {
                spacing: 4
                Text { text: "Emoji"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                Rectangle {
                    width: 56; height: 36
                    radius: Theme.radiusSm
                    color: emojiArea.containsMouse ? Theme.surfaceTop : Theme.surfaceHigh
                    border.color: Theme.border
                    Text {
                        anchors.centerIn: parent
                        text: root.emojiText
                        font.pixelSize: 20
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

        // Fields section header
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Campos"
                color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "+ Adicionar campo"
                onClicked: root.addField()
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : "transparent"
                    border.color: Theme.accentSoft
                    radius: Theme.radiusSm
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.accent
                    font.family: Theme.fontSans; font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Fields list
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            color: Theme.surfaceLow
            border.color: Theme.borderSoft
            radius: Theme.radiusSm

            ScrollView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    Repeater {
                        model: root.fields
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            property int rowIdx: index

                            TextField {
                                Layout.preferredWidth: 110
                                text: modelData.label
                                onTextChanged: if (text !== modelData.label)
                                                   root.patchField(rowIdx, { label: text })
                                placeholderText: "label"
                                color: Theme.fg
                                font.family: Theme.fontMono
                                font.pixelSize: 11
                                background: Rectangle {
                                    color: Theme.surfaceHigh
                                    border.color: parent.activeFocus ? Theme.accent : Theme.border
                                    radius: Theme.radiusSm
                                }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: modelData.value
                                onTextChanged: if (text !== modelData.value)
                                                   root.patchField(rowIdx, { value: text })
                                placeholderText: "valor"
                                color: Theme.fg
                                font.family: Theme.fontMono
                                font.pixelSize: 12
                                echoMode: modelData.masked ? TextInput.Password : TextInput.Normal
                                background: Rectangle {
                                    color: Theme.surfaceHigh
                                    border.color: parent.activeFocus ? Theme.accent : Theme.border
                                    radius: Theme.radiusSm
                                }
                            }
                            CheckBox {
                                text: "🙈 mascarado"
                                checked: modelData.masked
                                onCheckedChanged: if (checked !== modelData.masked)
                                                      root.patchField(rowIdx, { masked: checked })
                                font.pixelSize: 11
                                contentItem: Text {
                                    text: parent.text
                                    color: Theme.fgMuted
                                    font.pixelSize: 11
                                    leftPadding: parent.indicator.width + 4
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            Button {
                                id: removeBtn
                                text: "×"
                                onClicked: root.removeFieldAt(rowIdx)
                                implicitWidth: 26; implicitHeight: 26
                                background: Rectangle {
                                    color: removeBtn.hovered ? Theme.danger : "transparent"
                                    border.color: Theme.border
                                    radius: Theme.radiusSm
                                }
                                contentItem: Text {
                                    text: removeBtn.text
                                    color: removeBtn.hovered ? Theme.fgStrong : Theme.fgMuted
                                    font.pixelSize: 16
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }

                    Text {
                        visible: root.fields.length === 0
                        text: "Nenhum campo. Clique em \"+ Adicionar campo\"."
                        color: Theme.fgFaint
                        font.pixelSize: 11
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 30
                    }
                }
            }
        }

        // Notes
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: "Notas"; color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: Theme.surfaceHigh
                border.color: notesArea.activeFocus ? Theme.accent : Theme.border
                radius: Theme.radiusSm
                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 6
                    TextArea {
                        id: notesArea
                        text: root.notesText
                        onTextChanged: root.notesText = text
                        wrapMode: TextEdit.Wrap
                        color: Theme.fg
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                        background: null
                        selectByMouse: true
                    }
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
                enabled: root.isValid()
                onClicked: {
                    root.saved({
                        id: root.existingId,
                        name: root.nameText.trim(),
                        kind: root.kindIdx,
                        emoji: root.emojiText,
                        notes: root.notesText,
                        fields: root.fields,
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
