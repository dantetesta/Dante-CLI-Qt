// Floating AI Assistant panel — slides in from the right edge.
//
// Mirrors the Swift sibling's ExplainModal flow but as a persistent chat
// (analogous to ChatGPT-style overlay). The C++ `AIController` exposed as
// the `ai` context property owns state; this file is presentation + input.
//
// Wire-up (handled in Main.qml):
//   ai.insertRequested(text) → terminal.write(appState.activeTabId, text)
//
// Layout:
//   ┌────────────────────────────┐
//   │ ✨ Dante AI       [×] [⌫]  │  ← header
//   ├────────────────────────────┤
//   │  user / assistant bubbles  │  ← scrollable list
//   │                            │
//   ├────────────────────────────┤
//   │ Quick actions row          │
//   ├────────────────────────────┤
//   │ TextArea | 🎤 | ▶          │  ← composer
//   └────────────────────────────┘
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: root

    // Caller sets these (Main.qml).
    property int panelWidth: 420
    property int toolbarHeight: 42

    // Signal for the parent to wire to whatever voice agent eventually lands.
    signal voiceRequested()

    // Mounted in the same coordinate space as the main window, so we anchor
    // ourselves to fill it and use an inner Rectangle as the actual panel.
    anchors.fill: parent
    visible: ai.open || panel.x < parent.width
    z: 100

    // Click-through scrim — clicking outside the panel closes it.
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: ai.open ? 0.28 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }
        MouseArea {
            anchors.fill: parent
            enabled: ai.open
            onClicked: ai.hide()
        }
    }

    // The actual sliding panel.
    Rectangle {
        id: panel
        width: root.panelWidth
        height: root.height - root.toolbarHeight
        x: ai.open ? (root.width - width) : root.width
        y: 0
        color: Qt.rgba(0.125, 0.125, 0.145, 0.96)   // Theme.surfaceHigh @ ~0.92
        border.color: Theme.border
        border.width: 1
        radius: 0

        Behavior on x { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        // Eat clicks so the scrim's MouseArea doesn't fire.
        MouseArea { anchors.fill: parent; onClicked: {} }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 0
            spacing: 0

            // ─────────────── Header ───────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                color: Qt.rgba(0.105, 0.105, 0.125, 0.98)

                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1; color: Theme.borderSoft
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 8
                    spacing: 8

                    Text {
                        text: "✨"
                        font.pixelSize: 16
                        color: Theme.fgStrong
                    }
                    Text {
                        text: "Dante AI"
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.leftMargin: 6
                        text: ai.status === "thinking" ? "pensando…"
                            : ai.status === "no_api_key" ? "sem API key"
                            : ai.status === "error" ? "erro" : ""
                        color: ai.status === "error" ? Theme.danger
                             : ai.status === "no_api_key" ? Theme.warn
                             : Theme.fgFaint
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    IconButton {
                        text: "⌫"
                        tooltip: "Limpar conversa"
                        onClicked: ai.clearHistory()
                    }
                    IconButton {
                        text: "×"
                        tooltip: "Fechar"
                        onClicked: ai.hide()
                    }
                }
            }

            // ─────────────── Messages list (or empty/no-key state) ───────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // No-API-key empty state.
                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    visible: ai.status === "no_api_key" && ai.messages.length === 0
                    spacing: 10

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "🔑"
                        font.pixelSize: 34
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Configure sua Groq API key"
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Abra Configurações e cole sua chave em " +
                              "<i>Groq API key</i>. Você pode gerar uma " +
                              "gratuitamente em " +
                              "<a href=\"https://console.groq.com\">console.groq.com</a>."
                        textFormat: Text.RichText
                        wrapMode: Text.WordWrap
                        color: Theme.fgMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                    }
                }

                // Generic empty state.
                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    visible: ai.messages.length === 0 && ai.status !== "no_api_key"
                    spacing: 8
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "✨"
                        font.pixelSize: 32
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Pergunte algo ou peça um comando."
                        color: Theme.fgMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                ListView {
                    id: msgList
                    anchors.fill: parent
                    anchors.topMargin: 8
                    anchors.bottomMargin: 8
                    clip: true
                    spacing: 8
                    visible: ai.messages.length > 0
                    model: ai.messages

                    delegate: AIMessage {
                        width: msgList.width
                        role:      (modelData && modelData.role) ? modelData.role : "assistant"
                        content:   (modelData && modelData.content) ? modelData.content : ""
                        timestamp: (modelData && modelData.timestamp) ? modelData.timestamp : ""
                    }

                    onCountChanged: Qt.callLater(positionViewAtEnd)

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
            }

            // ─────────────── Quick actions ───────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                color: "transparent"

                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1; color: Theme.borderSoft
                }

                Flow {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    QuickAction {
                        label: "Explicar último comando"
                        onTriggered: ai.send("Explique brevemente o comando ou output mais recente que rodei.")
                    }
                    QuickAction {
                        label: "Sugerir comando"
                        onTriggered: composer.forceActiveFocus()
                    }
                    QuickAction {
                        label: "Reescrever comando"
                        onTriggered: ai.send("Reescreva o último comando que sugeri de forma mais idiomática e segura.")
                    }
                }
            }

            // ─────────────── Composer ───────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(140, Math.max(56, composerCol.implicitHeight + 16))
                color: Qt.rgba(0.07, 0.07, 0.09, 0.85)

                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1; color: Theme.borderSoft
                }

                RowLayout {
                    id: composerCol
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Theme.surfaceLow
                        border.color: composer.activeFocus ? Theme.accentSoft : Theme.border
                        border.width: 1
                        radius: Theme.radiusSm

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 2
                            TextArea {
                                id: composer
                                placeholderText: ai.status === "no_api_key"
                                    ? "Configure a Groq API key em Configurações…"
                                    : "Pergunte ao Dante… (Enter envia, Shift+Enter quebra linha)"
                                placeholderTextColor: Theme.fgFaint
                                color: Theme.fg
                                selectByMouse: true
                                wrapMode: TextArea.Wrap
                                font.family: Theme.fontSans
                                font.pixelSize: 13
                                background: null
                                enabled: ai.status !== "no_api_key"

                                Keys.onPressed: function(event) {
                                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                                            && !(event.modifiers & Qt.ShiftModifier)) {
                                        event.accepted = true
                                        sendCurrent()
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 32
                        Layout.fillHeight: true
                        spacing: 4

                        IconButton {
                            text: "🎤"
                            tooltip: "Ditado por voz (em breve)"
                            Layout.alignment: Qt.AlignHCenter
                            onClicked: root.voiceRequested()
                        }

                        IconButton {
                            text: ai.status === "thinking" ? "…" : "➤"
                            tooltip: "Enviar (Enter)"
                            Layout.alignment: Qt.AlignHCenter
                            highlighted: true
                            enabled: composer.text.trim().length > 0
                                  && ai.status !== "thinking"
                                  && ai.status !== "no_api_key"
                            onClicked: sendCurrent()
                        }
                    }
                }
            }
        }

        function sendCurrent() {
            const t = composer.text.trim()
            if (t.length === 0) return
            if (ai.status === "thinking" || ai.status === "no_api_key") return
            ai.send(t)
            composer.clear()
        }
    }

    // ───── Local sub-components ─────

    component IconButton: Rectangle {
        property string text: ""
        property string tooltip: ""
        property bool   highlighted: false
        property bool   enabled: true
        signal clicked()

        implicitWidth: 28
        implicitHeight: 28
        radius: Theme.radiusSm
        color: !enabled ? "transparent"
             : highlighted ? (ma.containsMouse ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
             : (ma.containsMouse ? Theme.surfaceHigh : "transparent")
        border.color: highlighted ? Theme.accent
                     : (ma.containsMouse ? Theme.border : "transparent")
        border.width: 1
        opacity: enabled ? 1.0 : 0.45

        Text {
            anchors.centerIn: parent
            text: parent.text
            color: parent.highlighted ? Theme.fgStrong : Theme.fg
            font.family: Theme.fontSans
            font.pixelSize: 13
        }

        ToolTip.visible: ma.containsMouse && tooltip.length > 0
        ToolTip.text:    tooltip
        ToolTip.delay:   400

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            enabled: parent.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    component QuickAction: Rectangle {
        property string label: ""
        signal triggered()

        implicitWidth: txt.implicitWidth + 18
        implicitHeight: 24
        radius: 12
        color: ma2.containsMouse ? Theme.surfaceHigh : Theme.surface
        border.color: ma2.containsMouse ? Theme.borderStrong : Theme.border
        border.width: 1

        Text {
            id: txt
            anchors.centerIn: parent
            text: parent.label
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 11
        }
        MouseArea {
            id: ma2
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.triggered()
        }
    }
}
