// Cheatsheet popup — Ctrl+/ (Cmd+/ on macOS). Mirrors the Swift
// CheatsheetView.swift: a scrollable, grouped reference of every keyboard
// shortcut + a tiny "tip of the day" footer.
//
// Uses MovablePopup so the user can drag/resize while they read.
// Reachable from: ⌘/ shortcut in Main.qml + "?" tooltip in BottomToolbar.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

MovablePopup {
    id: root
    width: 720
    height: 620
    minWidth: 520
    minHeight: 420
    title: qsTr("Atalhos de teclado")
    icon: "⌨"

    /// Curated catalogue. Order matters — first group renders first.
    readonly property var groups: [
        {
            label: qsTr("Sessão"),
            items: [
                { keys: "Ctrl+T",       desc: qsTr("Nova aba") },
                { keys: "Ctrl+W",       desc: qsTr("Fechar aba") },
                { keys: "Ctrl+Shift+W", desc: qsTr("Fechar painel focado") },
                { keys: "Ctrl+Shift+]", desc: qsTr("Próxima aba") },
                { keys: "Ctrl+Shift+[", desc: qsTr("Aba anterior") },
                { keys: "Ctrl+1…9",     desc: qsTr("Pular para aba N (em breve)") }
            ]
        },
        {
            label: qsTr("Splits & layout"),
            items: [
                { keys: "Ctrl+\\",       desc: qsTr("Split vertical") },
                { keys: "Ctrl+Shift+\\", desc: qsTr("Split horizontal") },
                { keys: "Ctrl+Alt+←",   desc: qsTr("Focar painel à esquerda") },
                { keys: "Ctrl+Alt+→",   desc: qsTr("Focar painel à direita") }
            ]
        },
        {
            label: qsTr("Assistente IA & Voz"),
            items: [
                { keys: "Ctrl+Shift+A", desc: qsTr("Abrir/fechar AI overlay") },
                { keys: "Ctrl+K",       desc: qsTr("Paleta de comandos (fuzzy)") },
                { keys: qsTr("(toolbar)"), desc: qsTr("🎤 inicia/para gravação para Whisper") }
            ]
        },
        {
            label: qsTr("Ajuda & janela"),
            items: [
                { keys: "Ctrl+/",       desc: qsTr("Mostrar esta cheatsheet") },
                { keys: "Esc",          desc: qsTr("Fechar modal aberto") }
            ]
        },
        {
            label: qsTr("File tree (Pastas)"),
            items: [
                { keys: qsTr("Duplo-clique"),  desc: qsTr("Entrar na pasta (vira raiz)") },
                { keys: qsTr("Botão direito"), desc: qsTr("Menu: abrir, renomear, duplicar, lixeira…") },
                { keys: qsTr("Arrastar"),      desc: qsTr("Arquivo/pasta → slot do grid") }
            ]
        }
    ]

    ScrollView {
        anchors.fill: parent
        anchors.margins: 14
        clip: true

        ColumnLayout {
            width: parent ? parent.width - 30 : 0
            spacing: 14

            Repeater {
                model: root.groups
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    // Group header.
                    Text {
                        text: modelData.label
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        Layout.bottomMargin: 2
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.borderSoft
                        opacity: 0.5
                    }

                    Repeater {
                        model: modelData.items
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            spacing: 12

                            // Keychord chip — monospaced, surface tile.
                            Rectangle {
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: 24
                                radius: 4
                                color: Theme.surfaceLow
                                border.color: Theme.borderSoft
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.keys
                                    color: Theme.fg
                                    font.family: Theme.fontMono
                                    font.pixelSize: 11
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.desc
                                color: Theme.fgMuted
                                font.family: Theme.fontSans
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 6
                color: Theme.accentDim
                border.color: Theme.accentSoft
                Text {
                    anchors.fill: parent
                    anchors.margins: 8
                    text: qsTr("💡 Dica: solte o mouse em cima do título do modal pra mover, ou arraste a quina inferior direita pra redimensionar.")
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
