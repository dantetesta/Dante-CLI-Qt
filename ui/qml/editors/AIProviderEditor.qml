// AIProviderEditor — inline (non-modal) form used by AIProvidersTab.qml.
// Both "add" (after aiProviders.addProvider()) and "edit" reuse the same
// fields; the parent simply calls loadFromMap(...) with the current
// provider's record. Save emits `saved` with a flat object that
// AIProvidersTab routes into `aiProviders.updateProvider(...)`.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import ".."

Item {
    id: root

    /// Currently-edited provider id ("" = nothing loaded yet).
    property string providerId: ""
    property string nameText: ""
    property string baseUrlText: ""
    property string apiKeyText: ""
    property string modelText: ""
    property string kindText: "chat"
    property bool   enabledFlag: true
    /// Toggles the apiKey echo-mode (false = masked, true = plaintext).
    property bool   revealKey: false

    /// Curated suggestion list for the model combo. The user can still
    /// type any value because the combo is editable.
    readonly property var modelSuggestions: [
        "llama-3.3-70b-versatile",
        "llama-3.1-8b-instant",
        "mixtral-8x7b-32768",
        "gpt-4o-mini",
        "gpt-4o",
        "openrouter/auto",
        "whisper-large-v3-turbo",
        "whisper-large-v3",
    ]

    readonly property var kindOptions: [
        { value: "chat",    label: qsTr("Chat") },
        { value: "whisper", label: qsTr("Voz (Whisper)") },
        { value: "both",    label: qsTr("Chat + Voz") },
    ]

    signal saved(var props)
    signal cancelled()

    function loadFromMap(m) {
        providerId  = m && m.id      !== undefined ? m.id      : ""
        nameText    = m && m.name    !== undefined ? m.name    : ""
        baseUrlText = m && m.baseUrl !== undefined ? m.baseUrl : ""
        apiKeyText  = m && m.apiKey  !== undefined ? m.apiKey  : ""
        modelText   = m && m.model   !== undefined ? m.model   : ""
        kindText    = m && m.kind    !== undefined ? m.kind    : "chat"
        enabledFlag = m && m.enabled !== undefined ? m.enabled : true
        revealKey   = false
    }

    function reset() {
        loadFromMap(null)
    }

    function isValid() {
        return nameText.trim().length > 0 && baseUrlText.trim().length > 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        /* ─── Header ─── */
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: root.providerId.length > 0 ? "✨" : "➕"
                font.pixelSize: 18
            }
            Text {
                Layout.fillWidth: true
                text: root.providerId.length > 0 ? qsTr("Editar provider") : qsTr("Novo provider")
                color: Theme.fgStrong
                font.family: Theme.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            Switch {
                checked: root.enabledFlag
                onToggled: root.enabledFlag = checked
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Ativar este provider")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSoft
        }

        /* ─── Form ─── */

        // Name
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: qsTr("Nome"); color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            TextField {
                id: nameField
                Layout.fillWidth: true
                text: root.nameText
                onTextChanged: root.nameText = text
                placeholderText: qsTr("Ex: Groq, OpenAI, Together…")
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

        // Base URL
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: qsTr("Base URL"); color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            TextField {
                id: urlField
                Layout.fillWidth: true
                text: root.baseUrlText
                onTextChanged: root.baseUrlText = text
                placeholderText: "https://api.openai.com/v1"
                color: Theme.fg
                font.family: Theme.fontMono
                font.pixelSize: 12
                background: Rectangle {
                    color: Theme.surfaceHigh
                    border.color: urlField.activeFocus ? Theme.accent : Theme.border
                    radius: Theme.radiusSm
                }
            }
        }

        // API key with eye toggle
        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text { text: qsTr("API Key"); color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                TextField {
                    id: keyField
                    Layout.fillWidth: true
                    text: root.apiKeyText
                    onTextChanged: root.apiKeyText = text
                    placeholderText: "sk-... | gsk_..."
                    echoMode: root.revealKey ? TextInput.Normal : TextInput.Password
                    color: Theme.fg
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    background: Rectangle {
                        color: Theme.surfaceHigh
                        border.color: keyField.activeFocus ? Theme.accent : Theme.border
                        radius: Theme.radiusSm
                    }
                }
                Button {
                    id: eyeBtn
                    implicitWidth: 34
                    implicitHeight: 30
                    onClicked: root.revealKey = !root.revealKey
                    ToolTip.visible: hovered
                    ToolTip.text: root.revealKey ? qsTr("Ocultar chave") : qsTr("Mostrar chave")
                    background: Rectangle {
                        color: eyeBtn.hovered ? Theme.surfaceTop : Theme.surfaceHigh
                        border.color: Theme.border
                        radius: Theme.radiusSm
                    }
                    contentItem: Text {
                        text: root.revealKey ? "🙈" : "👁"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Armazenada em ai-providers.json (texto puro — mesma postura do credentials.json).")
                color: Theme.fgFaint
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }
        }

        // Model + Kind row
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Text { text: qsTr("Modelo"); color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                ComboBox {
                    id: modelBox
                    Layout.fillWidth: true
                    editable: true
                    model: root.modelSuggestions
                    // ComboBox.editText is the live input — keep our property
                    // in lock-step so the Save handler reads the latest value.
                    editText: root.modelText
                    onEditTextChanged: if (editText !== root.modelText) root.modelText = editText
                    onActivated: (idx) => root.modelText = modelSuggestions[idx]
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                }
            }

            ColumnLayout {
                spacing: 4
                Layout.preferredWidth: 200
                Text { text: qsTr("Tipo"); color: Theme.fgMuted; font.pixelSize: 11; font.weight: Font.DemiBold }
                ComboBox {
                    id: kindBox
                    Layout.fillWidth: true
                    model: root.kindOptions
                    textRole: "label"
                    valueRole: "value"
                    currentIndex: {
                        for (let i = 0; i < root.kindOptions.length; ++i)
                            if (root.kindOptions[i].value === root.kindText) return i
                        return 0
                    }
                    onActivated: (idx) => root.kindText = root.kindOptions[idx].value
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                }
            }
        }

        Item { Layout.fillHeight: true }

        // Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Cancelar")
                onClicked: root.cancelled()
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : "transparent"
                    border.color: Theme.border
                    radius: Theme.radiusSm
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Button {
                id: saveBtn
                text: qsTr("Salvar")
                enabled: root.isValid()
                onClicked: {
                    root.saved({
                        id:      root.providerId,
                        name:    root.nameText.trim(),
                        baseUrl: root.baseUrlText.trim(),
                        apiKey:  root.apiKeyText,
                        model:   root.modelText.trim(),
                        kind:    root.kindText,
                        enabled: root.enabledFlag,
                    })
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
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 14
                    rightPadding: 14
                }
            }
        }
    }
}
