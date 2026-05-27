// AIProvidersTab — full-width settings tab that replaces the legacy "AI
// Providers" stub inside SettingsPanel.qml. Two-column layout:
//
//   ┌────────────────────────────┬─────────────────────────────────────┐
//   │ Provider list (+ defaults) │  AIProviderEditor (form)            │
//   │  ─ "+ Adicionar provider"  │   - name / baseUrl / apiKey / model │
//   │  ─ "Definir padrão (chat)" │   - kind / enabled                  │
//   │  ─ "Definir padrão (voz)"  │   - Save / Cancel                   │
//   └────────────────────────────┴─────────────────────────────────────┘
//
// Backed by the `aiProviders` context property (registered in main.cpp).
// Strictly QtQuick 6.5 + the local Theme singleton.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."
import "editors"

Item {
    id: root

    /// Currently-highlighted provider id ("" = nothing selected yet).
    property string selectedId: ""

    Component.onCompleted: {
        // Auto-select the first row once the delegates have realized. We
        // defer one tick because the ListView hasn't finished instantiating
        // its delegates inside Component.onCompleted — itemAtIndex(0) is
        // null until the next event-loop pass.
        if (providerList.count > 0) firstPickTimer.start()
    }

    Timer {
        id: firstPickTimer
        interval: 60
        repeat: false
        onTriggered: {
            // Read the provider id from the realized delegate (rowProviderId
            // is a property we expose on each row Rectangle).
            const item = providerList.itemAtIndex(0)
            if (item && item.rowProviderId) {
                root.selectedId = item.rowProviderId
                const m = aiProviders.get(item.rowProviderId)
                if (m) editor.loadFromMap(m)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        /* ─── Left column: list + actions ─── */
        Rectangle {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.borderSoft
            border.width: 1
            radius: Theme.radiusMd

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                /* ─── Action buttons ─── */
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Button {
                        id: addBtn
                        Layout.fillWidth: true
                        text: qsTr("+  Adicionar provider")
                        onClicked: {
                            const newId = aiProviders.addProvider()
                            root.selectedId = newId
                            const m = aiProviders.get(newId)
                            if (m) editor.loadFromMap(m)
                        }
                        background: Rectangle {
                            color: addBtn.hovered ? Theme.accent : Theme.accentSoft
                            radius: Theme.radiusSm
                        }
                        contentItem: Text {
                            text: parent.text
                            color: Theme.fgStrong
                            font.family: Theme.fontSans
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        id: defaultChatBtn
                        Layout.fillWidth: true
                        enabled: root.selectedId.length > 0
                        text: qsTr("Definir como padrão (chat)")
                        onClicked: aiProviders.pickDefaultChat(root.selectedId)
                        background: Rectangle {
                            color: defaultChatBtn.enabled
                                ? (defaultChatBtn.hovered ? Theme.surfaceTop : Theme.surfaceHigh)
                                : Theme.surfaceHigh
                            border.color: Theme.border
                            radius: Theme.radiusSm
                        }
                        contentItem: Text {
                            text: parent.text
                            color: defaultChatBtn.enabled ? Theme.fg : Theme.fgFaint
                            font.family: Theme.fontSans
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        id: defaultVoiceBtn
                        Layout.fillWidth: true
                        enabled: root.selectedId.length > 0
                        text: qsTr("Definir como padrão (voz)")
                        onClicked: aiProviders.pickDefaultWhisper(root.selectedId)
                        background: Rectangle {
                            color: defaultVoiceBtn.enabled
                                ? (defaultVoiceBtn.hovered ? Theme.surfaceTop : Theme.surfaceHigh)
                                : Theme.surfaceHigh
                            border.color: Theme.border
                            radius: Theme.radiusSm
                        }
                        contentItem: Text {
                            text: parent.text
                            color: defaultVoiceBtn.enabled ? Theme.fg : Theme.fgFaint
                            font.family: Theme.fontSans
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSoft
                }

                /* ─── Provider list ─── */
                ListView {
                    id: providerList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: aiProviders

                    delegate: Rectangle {
                        id: row
                        // Expose providerId so the timer-based "auto-pick
                        // first row" can read it without round-tripping
                        // through the model's role API in QML.
                        property string rowProviderId: providerId
                        width: providerList.width
                        height: 64
                        radius: Theme.radiusSm
                        color: providerId === root.selectedId
                            ? Theme.accentDim
                            : (rowArea.containsMouse ? Theme.surfaceHigh : Theme.surfaceLow)
                        border.color: providerId === root.selectedId ? Theme.accentSoft : Theme.borderSoft
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Text {
                                    Layout.fillWidth: true
                                    text: name && name.length > 0 ? name : qsTr("(sem nome)")
                                    color: Theme.fgStrong
                                    font.family: Theme.fontSans
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }
                                Rectangle {
                                    visible: !enabled
                                    color: Theme.surfaceTop
                                    border.color: Theme.borderSoft
                                    radius: 4
                                    implicitWidth: offText.implicitWidth + 8
                                    implicitHeight: 14
                                    Text {
                                        id: offText
                                        anchors.centerIn: parent
                                        text: qsTr("off")
                                        color: Theme.fgFaint
                                        font.pixelSize: 9
                                        font.family: Theme.fontMono
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: baseUrl
                                color: Theme.fgMuted
                                font.family: Theme.fontMono
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                // kind chip
                                Rectangle {
                                    radius: 4
                                    color: Theme.surfaceTop
                                    border.color: Theme.borderSoft
                                    implicitWidth: kindLabel.implicitWidth + 8
                                    implicitHeight: 14
                                    Text {
                                        id: kindLabel
                                        anchors.centerIn: parent
                                        text: kind
                                        color: Theme.fgMuted
                                        font.pixelSize: 9
                                        font.family: Theme.fontMono
                                    }
                                }
                                // default chat badge
                                Rectangle {
                                    visible: isDefaultChat
                                    radius: 4
                                    color: Theme.accent
                                    implicitWidth: dcLabel.implicitWidth + 8
                                    implicitHeight: 14
                                    Text {
                                        id: dcLabel
                                        anchors.centerIn: parent
                                        text: qsTr("padrão chat")
                                        color: Theme.fgStrong
                                        font.pixelSize: 9
                                        font.family: Theme.fontSans
                                        font.weight: Font.DemiBold
                                    }
                                }
                                // default voice badge
                                Rectangle {
                                    visible: isDefaultWhisper
                                    radius: 4
                                    color: Theme.success
                                    implicitWidth: dvLabel.implicitWidth + 8
                                    implicitHeight: 14
                                    Text {
                                        id: dvLabel
                                        anchors.centerIn: parent
                                        text: qsTr("padrão voz")
                                        color: Theme.fgStrong
                                        font.pixelSize: 9
                                        font.family: Theme.fontSans
                                        font.weight: Font.DemiBold
                                    }
                                }
                                // hasApiKey marker — silent reassurance.
                                Rectangle {
                                    visible: hasApiKey
                                    radius: 4
                                    color: "transparent"
                                    border.color: Theme.borderStrong
                                    implicitWidth: kText.implicitWidth + 8
                                    implicitHeight: 14
                                    Text {
                                        id: kText
                                        anchors.centerIn: parent
                                        text: qsTr("key")
                                        color: Theme.fgMuted
                                        font.pixelSize: 9
                                        font.family: Theme.fontMono
                                    }
                                }
                                Item { Layout.fillWidth: true }
                                // Delete button — only on hover/selection to
                                // keep the chip clean for the common case.
                                Text {
                                    visible: row.color === Theme.accentDim || rowArea.containsMouse
                                    text: "🗑"
                                    color: rmArea.containsMouse ? Theme.danger : Theme.fgMuted
                                    font.pixelSize: 12
                                    MouseArea {
                                        id: rmArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            aiProviders.removeProvider(providerId)
                                            if (root.selectedId === providerId) {
                                                root.selectedId = ""
                                                editor.reset()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            id: rowArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            // Only the chrome handles the click — the delete
                            // text has its own MouseArea above, which captures
                            // first because it's a child of the row.
                            onClicked: {
                                root.selectedId = providerId
                                const m = aiProviders.get(providerId)
                                if (m) editor.loadFromMap(m)
                            }
                        }
                    }

                    // Empty state
                    Text {
                        anchors.centerIn: parent
                        visible: providerList.count === 0
                        text: qsTr("Nenhum provider.\nClique em \"+ Adicionar provider\".")
                        color: Theme.fgFaint
                        font.family: Theme.fontSans
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        /* ─── Right column: editor or empty state ─── */
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.borderSoft
            border.width: 1
            radius: Theme.radiusMd

            // Empty state when nothing is selected yet.
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.selectedId.length === 0
                spacing: 8
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "✨"
                    font.pixelSize: 36
                    color: Theme.fgFaint
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Selecione um provider à esquerda\nou clique em \"+ Adicionar provider\".")
                    color: Theme.fgMuted
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            AIProviderEditor {
                id: editor
                anchors.fill: parent
                visible: root.selectedId.length > 0
                onSaved: function(props) {
                    aiProviders.updateProvider(
                        props.id,
                        props.name,
                        props.baseUrl,
                        props.apiKey,
                        props.model,
                        props.kind,
                        props.enabled
                    )
                    // Reload from the model so any normalization the C++
                    // side applied (trimming etc.) is reflected back.
                    const fresh = aiProviders.get(props.id)
                    if (fresh) editor.loadFromMap(fresh)
                }
                onCancelled: {
                    // Discard pending edits — repopulate from the model.
                    const m = aiProviders.get(root.selectedId)
                    if (m) editor.loadFromMap(m)
                }
            }
        }
    }
}
