// Settings — 5-tab modal patterned on the Swift sibling's PreferencesWindow.
//   Geral · Aparência · Voz · AI Providers · Atualizações
// Tabs sit in a horizontal strip at the top of the window; the body switches
// via StackLayout. Each tab uses the SectionCard / Field pattern (rounded
// surface with internal divider lines) so it visually matches the Mac app.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Popup {
    id: root

    modal: true
    focus: true
    width: 820
    height: Math.min(720, parent ? parent.height - 80 : 720)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
        color: Theme.surfaceHigh
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
    }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionStd; easing.type: Easing.OutCubic } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    /// Index of the currently visible tab (0..4).
    property int currentTab: 0

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 0

        /* ─── Header with tab strip ─── */
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 86
            color: Theme.surface
            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                height: 1; color: Theme.borderSoft
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 8
                anchors.bottomMargin: 6
                spacing: 4

                // Section title (changes per tab).
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: tabModel[root.currentTab].label
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                // Tab strip — Geral / Aparência / Voz / AI Providers / Atualizações.
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 14
                    Repeater {
                        model: tabModel
                        delegate: TabButton {
                            tabIndex: index
                            iconText: modelData.icon
                            label: modelData.label
                        }
                    }
                }
            }
            // Close (top-right ×) — discoverable since this is a modal popup.
            Button {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                text: "×"
                flat: true
                width: 30; height: 30
                onClicked: root.close()
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : "transparent"
                    radius: 6
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.fgMuted
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        /* ─── Tab content ─── */
        StackLayout {
            id: tabStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentTab

            // 0 — Geral
            ScrollView {
                clip: true
                ColumnLayout {
                    width: tabStack.width
                    spacing: 18
                    Item { Layout.preferredHeight: 12 }

                    SectionCard {
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true

                            Field {
                                label: qsTr("Shell padrão")
                                Layout.fillWidth: true
                                TextField {
                                    Layout.preferredWidth: 240
                                    text: appState.shellOverride
                                    placeholderText: qsTr("(auto)")
                                    onEditingFinished: appState.shellOverride = text
                                    color: Theme.fg
                                    background: Rectangle { color: Theme.surfaceLow; border.color: Theme.border; radius: Theme.radiusSm }
                                }
                                Text {
                                    text: qsTr("/bin/zsh (auto se vazio)")
                                    color: Theme.fgFaint
                                    font.family: Theme.fontMono
                                    font.pixelSize: 11
                                    Layout.preferredWidth: 140
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                            Divider {}

                            Field {
                                label: qsTr("Histórico (scrollback)")
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                ComboBox {
                                    model: [10000, 25000, 50000, 100000, 250000]
                                    currentIndex: {
                                        const v = appState.scrollback
                                        for (let i = 0; i < model.length; ++i) if (model[i] === v) return i
                                        return 2  // default 50k
                                    }
                                    delegate: ItemDelegate {
                                        width: parent.width
                                        text: (modelData / 1000).toFixed(0) + qsTr(" mil linhas")
                                    }
                                    displayText: (currentValue / 1000).toFixed(0) + qsTr(" mil linhas")
                                    onActivated: (idx) => appState.scrollback = model[idx]
                                }
                            }
                            HintRow { text: qsTr("Quanto rola pra cima após cada comando. Aplicado a novas abas.") }
                            Divider {}

                            ToggleRow {
                                label: qsTr("Restaurar abas na próxima abertura (best-effort)")
                                checked: appState.restoreOnLaunch
                                onToggled: appState.restoreOnLaunch = !appState.restoreOnLaunch
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }

            // 1 — Aparência
            ScrollView {
                clip: true
                ColumnLayout {
                    width: tabStack.width
                    spacing: 18
                    Item { Layout.preferredHeight: 12 }

                    SectionTitle { text: qsTr("Janela") }
                    SectionCard {
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Field {
                                label: qsTr("Aparência")
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                ComboBox {
                                    model: [qsTr("Sistema"), qsTr("Escuro"), qsTr("Claro")]
                                    currentIndex: appState.appearanceMode
                                    onActivated: (idx) => appState.appearanceMode = idx
                                }
                            }
                        }
                    }

                    SectionTitle { text: qsTr("Terminal") }
                    SectionCard {
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Field {
                                label: qsTr("Esquema base")
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                ComboBox {
                                    model: schemes ? schemes.list() : []
                                    textRole: "name"
                                    valueRole: "id"
                                    currentIndex: {
                                        if (!schemes) return 0
                                        const list = schemes.list()
                                        for (let i = 0; i < list.length; ++i)
                                            if (list[i].id === appState.terminalScheme) return i
                                        return 0
                                    }
                                    onActivated: (idx) => {
                                        const list = schemes.list()
                                        if (list[idx]) appState.terminalScheme = list[idx].id
                                    }
                                }
                            }
                            Divider {}
                            Field {
                                label: qsTr("Fonte")
                                Layout.fillWidth: true
                                TextField {
                                    Layout.fillWidth: true
                                    text: appState.fontName
                                    placeholderText: Theme.fontMono
                                    onEditingFinished: appState.fontName = text
                                    color: Theme.fg
                                    background: Rectangle { color: Theme.surfaceLow; border.color: Theme.border; radius: Theme.radiusSm }
                                }
                            }
                            Divider {}
                            Field {
                                label: qsTr("Tamanho da fonte")
                                Layout.fillWidth: true
                                Slider {
                                    Layout.fillWidth: true
                                    from: 9; to: 28
                                    stepSize: 1
                                    snapMode: Slider.SnapAlways
                                    value: appState.fontSize
                                    onValueChanged: if (Math.round(value) !== appState.fontSize)
                                                        appState.fontSize = Math.round(value)
                                }
                                Text {
                                    text: appState.fontSize + "pt"
                                    color: Theme.fgMuted
                                    font.family: Theme.fontMono
                                    font.pixelSize: 11
                                    Layout.preferredWidth: 40
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }

            // 2 — Voz
            ScrollView {
                clip: true
                ColumnLayout {
                    width: tabStack.width
                    spacing: 18
                    Item { Layout.preferredHeight: 12 }

                    SectionTitle { text: qsTr("Groq Whisper API") }
                    SectionCard {
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Field {
                                label: qsTr("API Key")
                                Layout.fillWidth: true
                                TextField {
                                    Layout.fillWidth: true
                                    text: appState.groqApiKey
                                    placeholderText: "gsk_..."
                                    echoMode: TextInput.Password
                                    onEditingFinished: appState.groqApiKey = text
                                    color: Theme.fg
                                    background: Rectangle { color: Theme.surfaceLow; border.color: Theme.border; radius: Theme.radiusSm }
                                }
                            }
                            HintRow {
                                text: qsTr("Criar API key gratuita em console.groq.com")
                                color: Theme.accent
                            }
                            Divider {}
                            Field {
                                label: qsTr("Modelo")
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                ComboBox {
                                    model: ["whisper-large-v3-turbo", "whisper-large-v3"]
                                    currentIndex: model.indexOf(appState.voiceModel) >= 0
                                        ? model.indexOf(appState.voiceModel) : 0
                                    onActivated: (idx) => appState.voiceModel = model[idx]
                                }
                            }
                            Divider {}
                            Field {
                                label: qsTr("Idioma")
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                ComboBox {
                                    textRole: "label"
                                    valueRole: "code"
                                    model: ListModel {
                                        ListElement { label: "Português"; code: "pt" }
                                        ListElement { label: "English";   code: "en" }
                                        ListElement { label: "Español";   code: "es" }
                                        ListElement { label: "Français";  code: "fr" }
                                        ListElement { label: "Auto";      code: "" }
                                    }
                                    currentIndex: {
                                        const langs = ["pt","en","es","fr",""]
                                        const i = langs.indexOf(appState.voiceLanguage)
                                        return i >= 0 ? i : 0
                                    }
                                    onActivated: (idx) => {
                                        const langs = ["pt","en","es","fr",""]
                                        appState.voiceLanguage = langs[idx]
                                    }
                                }
                            }
                            Divider {}
                            ToggleRow {
                                label: qsTr("Submeter automaticamente (Enter após injetar)")
                                checked: appState.voiceAutoSubmit
                                onToggled: appState.voiceAutoSubmit = !appState.voiceAutoSubmit
                            }
                        }
                    }

                    SectionTitle { text: qsTr("Como funciona") }
                    SectionCard {
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Layout.leftMargin: 0; Layout.rightMargin: 0
                            Text {
                                Layout.fillWidth: true
                                Layout.margins: 14
                                wrapMode: Text.WordWrap
                                text: qsTr("Click no botão de microfone na toolbar pra começar gravar. Click de novo pra parar — o áudio é enviado pro Groq Whisper e a transcrição é injetada na aba ativa do terminal.\n\nFree tier do Groq: 6.000 minutos/dia.")
                                color: Theme.fgMuted
                                font.family: Theme.fontSans
                                font.pixelSize: 12
                                lineHeight: 1.4
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }

            // 3 — AI Providers (placeholder — providers are still hardcoded
            // in the toolbar; the editor lands once the backend stores them).
            ScrollView {
                clip: true
                ColumnLayout {
                    width: tabStack.width
                    spacing: 18
                    Item { Layout.preferredHeight: 12 }

                    Text {
                        Layout.leftMargin: 18; Layout.rightMargin: 18
                        Layout.fillWidth: true
                        text: qsTr("Configure as CLIs de IA exibidas na toolbar.")
                        color: Theme.fgMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                    }
                    SectionCard {
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Layout.margins: 14

                            // Hardcoded preview rows mirroring the Swift defaults.
                            Repeater {
                                model: [
                                    {name: "Claude",  cmd: "claude",  icon: "🤖", colorIdx: 2},
                                    {name: "Gemini",  cmd: "gemini",  icon: "✨", colorIdx: 6},
                                    {name: "Codex",   cmd: "codex",   icon: "💻", colorIdx: 4}
                                ]
                                delegate: ProviderRow {
                                    name: modelData.name
                                    command: modelData.cmd
                                    icon: modelData.icon
                                    colorIdx: modelData.colorIdx
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: 4
                                Button {
                                    text: qsTr("Adicionar Provider")
                                    enabled: false
                                    onClicked: { /* TODO: backend */ }
                                }
                                Button {
                                    text: qsTr("Resetar para padrão")
                                    flat: true
                                    enabled: false
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: qsTr("(edição em breve)")
                                    color: Theme.fgFaint
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }

            // 4 — Atualizações
            ScrollView {
                clip: true
                ColumnLayout {
                    width: tabStack.width
                    spacing: 18
                    Item { Layout.preferredHeight: 12 }

                    SectionCard {
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.margins: 14
                            spacing: 14

                            Rectangle {
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64
                                radius: 14
                                color: Theme.accent
                                Text {
                                    anchors.centerIn: parent
                                    text: "⬇"
                                    color: "white"
                                    font.pixelSize: 28
                                    font.bold: true
                                }
                            }
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Text {
                                    text: "DANTE CLI"
                                    color: Theme.fgStrong
                                    font.family: Theme.fontSans
                                    font.pixelSize: 16
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: qsTr("Versão %1").arg(Qt.application.version)
                                    color: Theme.fgMuted
                                    font.family: Theme.fontMono
                                    font.pixelSize: 12
                                }
                                Text {
                                    text: qsTr("Manifest: dantetesta.com.br/dante-cli/winupdate.json")
                                    color: Theme.fgFaint
                                    font.family: Theme.fontMono
                                    font.pixelSize: 10
                                }
                            }
                            Button {
                                text: "↻  " + qsTr("Verificar")
                                onClicked: updater.checkNow()
                                background: Rectangle {
                                    radius: 8
                                    color: parent.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    font.family: Theme.fontSans
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    leftPadding: 16; rightPadding: 16
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }

                    SectionTitle { text: qsTr("Verificação automática") }
                    SectionCard {
                        ToggleRow {
                            label: qsTr("Verificar ao abrir o app")
                            sublabel: qsTr("Checa se há nova versão sempre que o Dante CLI inicia.")
                            checked: appState.autoCheckUpdates
                            onToggled: appState.autoCheckUpdates = !appState.autoCheckUpdates
                        }
                    }

                    SectionTitle { text: qsTr("Status") }
                    SectionCard {
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.margins: 14
                            spacing: 12
                            Rectangle {
                                width: 28; height: 28; radius: 14
                                color: updater && updater.available ? Theme.warn : Theme.success
                                Text {
                                    anchors.centerIn: parent
                                    text: updater && updater.available ? "!" : "✓"
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Text {
                                    text: updater && updater.available
                                        ? qsTr("Nova versão disponível")
                                        : qsTr("Está na última versão")
                                    color: Theme.fgStrong
                                    font.family: Theme.fontSans
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                }
                                Text {
                                    text: updater && updater.available
                                        ? qsTr("%1 é a mais recente disponível.").arg(updater.info.version || "")
                                        : qsTr("Você está rodando %1.").arg(Qt.application.version)
                                    color: Theme.fgMuted
                                    font.family: Theme.fontSans
                                    font.pixelSize: 11
                                }
                            }
                            Button {
                                visible: updater && updater.available
                                text: qsTr("Atualizar agora")
                                onClicked: updater.downloadAndInstall()
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 12 }
                }
            }
        }
    }

    /* ─── Tab list (centralized to keep the strip + title in sync) ─── */
    property var tabModel: [
        { label: qsTr("Geral"),         icon: "⚙" },
        { label: qsTr("Aparência"),     icon: "🎨" },
        { label: qsTr("Voz"),           icon: "🎤" },
        { label: qsTr("AI Providers"),  icon: "✨" },
        { label: qsTr("Atualizações"),  icon: "⬇" }
    ]

    /* ─── Inline components ─── */
    component TabButton: Item {
        property int    tabIndex
        property string iconText
        property string label
        readonly property bool active: root.currentTab === tabIndex
        implicitWidth: tCol.implicitWidth + 16
        implicitHeight: 56

        Rectangle {
            anchors.fill: parent
            radius: 8
            color: active ? Theme.surfaceTop : (ma.containsMouse ? Theme.surfaceHigh : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
        }
        ColumnLayout {
            id: tCol
            anchors.centerIn: parent
            spacing: 2
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: iconText
                color: active ? Theme.accent : Theme.fgMuted
                font.pixelSize: 18
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: label
                color: active ? Theme.accent : Theme.fgMuted
                font.family: Theme.fontSans
                font.pixelSize: 11
                font.weight: active ? Font.DemiBold : Font.Normal
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.currentTab = tabIndex
        }
    }

    component SectionTitle: Text {
        Layout.leftMargin: 18; Layout.rightMargin: 18
        Layout.fillWidth: true
        color: Theme.fgStrong
        font.family: Theme.fontSans
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    component SectionCard: Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 18; Layout.rightMargin: 18
        Layout.preferredHeight: childrenRect.height + 24
        radius: Theme.radiusMd
        color: Theme.surface
        border.color: Theme.borderSoft
        border.width: 1
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Theme.borderSoft
        opacity: 0.6
    }

    component Field: RowLayout {
        property string label
        Layout.fillWidth: true
        Layout.margins: 14
        spacing: 12
        Text {
            text: label
            color: Theme.fg
            font.family: Theme.fontSans
            font.pixelSize: 12
            Layout.preferredWidth: 180
        }
    }

    component HintRow: Text {
        property color color: Theme.fgFaint
        Layout.fillWidth: true
        Layout.leftMargin: 14; Layout.rightMargin: 14
        Layout.bottomMargin: 6
        font.family: Theme.fontSans
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }

    component ToggleRow: RowLayout {
        property string label
        property string sublabel: ""
        property bool   checked
        signal toggled()
        Layout.fillWidth: true
        Layout.margins: 14
        spacing: 12
        ColumnLayout {
            spacing: 2
            Layout.fillWidth: true
            Text {
                text: label
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 12
            }
            Text {
                visible: sublabel.length > 0
                text: sublabel
                color: Theme.fgFaint
                font.family: Theme.fontSans
                font.pixelSize: 11
            }
        }
        Switch {
            checked: parent.checked
            onToggled: parent.toggled()
        }
    }

    component ProviderRow: Rectangle {
        property string name
        property string command
        property string icon
        property int    colorIdx: 0
        readonly property var swatchColors: ["#9B9BAA","#DC2626","#F59E0B","#FACC15","#22C55E","#10B981","#3B82F6"]
        Layout.fillWidth: true
        Layout.preferredHeight: 64
        radius: Theme.radiusSm
        color: Theme.surfaceLow
        border.color: Theme.borderSoft

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4
            RowLayout {
                spacing: 8
                CheckBox { checked: true; enabled: false }
                TextField {
                    Layout.preferredWidth: 120
                    text: name
                    enabled: false
                    color: Theme.fg
                    background: Rectangle { color: Theme.surface; border.color: Theme.borderSoft; radius: 4 }
                }
                TextField {
                    Layout.fillWidth: true
                    text: command
                    enabled: false
                    color: Theme.fg
                    background: Rectangle { color: Theme.surface; border.color: Theme.borderSoft; radius: 4 }
                }
            }
            RowLayout {
                spacing: 6
                Text { text: icon; font.pixelSize: 16 }
                Text { text: qsTr("Trocar"); color: Theme.accent; font.pixelSize: 11 }
                Item { Layout.preferredWidth: 12 }
                Repeater {
                    model: swatchColors
                    delegate: Rectangle {
                        width: 16; height: 16; radius: 8
                        color: modelData
                        border.color: index === colorIdx ? Theme.fgStrong : "transparent"
                        border.width: 2
                    }
                }
            }
        }
    }
}
