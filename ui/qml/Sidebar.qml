// 4-mode sidebar: Favoritos / Pastas / Snippets / Chaves.
//
// Owns the inline editors (favorites/snippets/credentials) for create+edit.
// Editor popups mount at root level so they overlay the entire sidebar and
// don't get clipped by ListView delegates.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."
import "editors"

Rectangle {
    id: root
    color: Theme.surface

    Rectangle {                          // right divider
        anchors.right: parent.right
        anchors.top: parent.top; anchors.bottom: parent.bottom
        width: 1
        color: Theme.borderSoft
    }

    /* ─────────── Editors ─────────── */
    FavoriteEditor {
        id: favEditor
        onSaved: function(props) {
            if (props.id && props.id.length > 0) {
                favorites.update(props.id, props)
            } else {
                favorites.addFull(props)
            }
        }
    }
    SnippetEditor {
        id: snipEditor
        onSaved: function(props) {
            if (props.id && props.id.length > 0) {
                snippets.update(props.id, props)
            } else {
                snippets.addFull(props)
            }
        }
    }
    CredentialEditor {
        id: credEditor
        onSaved: function(props) {
            if (props.id && props.id.length > 0) {
                credentials.update(props.id, props)
            } else {
                // add() only accepts (name, kind, emoji, fields); notes is
                // patched on the next edit. The Swift sibling has the same
                // shape — notes was added late in v1.4 and the create flow
                // never grew the param. TODO: thread notes through add().
                credentials.add(props.name, props.kind, props.emoji, props.fields)
            }
        }
    }

    /// Open the right editor for the current sidebar mode.
    /// `id` empty → create; non-empty → edit existing record.
    function openEditorForCurrentMode(id) {
        const m = appState.sidebarMode
        if (m === 0) {
            favEditor.reset()
            if (id && id.length > 0) favEditor.loadFromMap(favorites.get(id))
            favEditor.open()
        } else if (m === 2) {
            snipEditor.reset()
            if (id && id.length > 0) snipEditor.loadFromMap(snippets.get(id))
            snipEditor.open()
        } else if (m === 3) {
            credEditor.reset()
            if (id && id.length > 0) credEditor.loadFromMap(credentials.get(id))
            credEditor.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        /* ───── Brand row ───── */
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                Rectangle {
                    width: 32; height: 32; radius: 6
                    color: Theme.accentDim
                    border.color: Theme.accentSoft
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: "D"
                        color: Theme.accent
                        font.pixelSize: 18
                        font.bold: true
                    }
                }
                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Text { text: "Dante CLI"; color: Theme.fgStrong; font.family: Theme.fontSans; font.pixelSize: 14; font.weight: Font.DemiBold }
                    Text { text: "Terminal premium"; color: Theme.fgFaint; font.family: Theme.fontSans; font.pixelSize: 11 }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSoft }

        /* ───── Mode picker ───── */
        Rectangle {
            Layout.fillWidth: true
            Layout.margins: 12
            Layout.preferredHeight: 40
            color: Theme.surfaceLow
            border.color: Theme.borderSoft
            radius: Theme.radiusMd
            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4
                Repeater {
                    model: [
                        { glyph: "★", label: "Favoritos",  idx: 0 },
                        { glyph: "📁", label: "Pastas",    idx: 1 },
                        { glyph: "⚡", label: "Snippets",  idx: 2 },
                        { glyph: "🔑", label: "Chaves",    idx: 3 },
                    ]
                    Rectangle {
                        id: modeBtn
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        readonly property bool selected: appState.sidebarMode === modelData.idx
                        color: selected ? Theme.accentDim
                                        : (modeArea.containsMouse ? Theme.surfaceHigh : "transparent")
                        border.color: selected ? Theme.accentSoft
                                               : (modeArea.containsMouse ? Theme.borderSoft : "transparent")
                        scale: modeArea.pressed ? 0.94 : 1.0
                        transformOrigin: Item.Center

                        Behavior on color        { ColorAnimation  { duration: 160; easing.type: Easing.OutCubic } }
                        Behavior on border.color { ColorAnimation  { duration: 160; easing.type: Easing.OutCubic } }
                        Behavior on scale        { NumberAnimation { duration: 110; easing.type: Easing.OutQuad  } }

                        Text {
                            anchors.centerIn: parent
                            text: modelData.glyph
                            font.pixelSize: 14
                            color: modeBtn.selected ? Theme.accent : Theme.fgMuted
                            Behavior on color { ColorAnimation { duration: 160; easing.type: Easing.OutCubic } }
                        }
                        MouseArea {
                            id: modeArea
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: appState.sidebarMode = modelData.idx
                            ToolTip.text: modelData.label
                            ToolTip.visible: containsMouse
                            hoverEnabled: true
                        }
                    }
                }
            }
        }

        /* ───── Search + "add" button ─────
           The "+" button sits to the right of the search field and is
           hidden on the Files tab (mode 1) since there's no editor for
           the filesystem tree. */
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12; Layout.rightMargin: 12
            spacing: 6

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: Theme.surfaceHigh
                border.color: Theme.border
                radius: Theme.radiusMd

                TextField {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    placeholderText: "Buscar… (⌘L)"
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 13
                    background: null
                    onTextChanged: appState.filter = text
                }
            }

            Rectangle {
                visible: appState.sidebarMode !== 1
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: Theme.radiusMd
                color: addArea.containsMouse ? Theme.accent : Theme.accentSoft
                border.color: Theme.accentSoft
                scale: addArea.pressed ? 0.94 : 1.0
                transformOrigin: Item.Center
                Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutQuad } }
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    color: Theme.fgStrong
                    font.pixelSize: 18
                    font.bold: true
                }
                MouseArea {
                    id: addArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.openEditorForCurrentMode("")
                    ToolTip.text: appState.sidebarMode === 0 ? "Novo favorito"
                                : appState.sidebarMode === 2 ? "Novo snippet"
                                : "Nova credencial"
                    ToolTip.visible: containsMouse
                }
            }
        }

        /* ───── Body: per-mode list ───── */
        StackLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            Layout.topMargin: 8
            currentIndex: appState.sidebarMode

            // 0 — Favoritos
            ListView {
                model: favorites
                clip: true
                spacing: 1
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 44
                    color: ma.containsMouse ? Theme.surfaceHigh : "transparent"
                    Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text { text: model.emoji && model.emoji.length > 0 ? model.emoji : "📁"; font.pixelSize: 14 }
                        ColumnLayout {
                            spacing: 2; Layout.fillWidth: true
                            Text { text: model.name; color: Theme.fg; font.pixelSize: 13; font.weight: Font.Medium; elide: Text.ElideRight }
                            Text { text: model.path; color: Theme.fgFaint; font.pixelSize: 11; font.family: Theme.fontMono; elide: Text.ElideMiddle }
                        }
                    }
                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onDoubleClicked: terminal.write(appState.activeTabId, "cd \"" + model.path + "\"\r")
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) favMenu.popup()
                        }
                    }
                    Menu {
                        id: favMenu
                        MenuItem {
                            text: "Editar"
                            onTriggered: root.openEditorForCurrentMode(model.favId)
                        }
                        MenuItem {
                            text: "Excluir"
                            onTriggered: favorites.remove(model.favId)
                        }
                    }
                }
            }

            // 1 — Pastas (placeholder). Wrapped in an Item so the layout-managed
            // StackLayout doesn't choke on anchors-on-leaf-Text.
            Item {
                Text {
                    text: "Files tree (em breve)"
                    color: Theme.fgFaint
                    anchors.centerIn: parent
                    font.pixelSize: 12
                }
            }

            // 2 — Snippets
            ListView {
                model: snippets
                clip: true
                spacing: 1
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 44
                    color: sma.containsMouse ? Theme.surfaceHigh : "transparent"
                    Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text { text: model.emoji && model.emoji.length > 0 ? model.emoji : "⚡"; font.pixelSize: 14 }
                        ColumnLayout {
                            spacing: 2; Layout.fillWidth: true
                            Text { text: model.name; color: Theme.fg; font.pixelSize: 13; elide: Text.ElideRight }
                            Text { text: model.command; color: Theme.fgFaint; font.pixelSize: 11; font.family: Theme.fontMono; elide: Text.ElideRight }
                        }
                    }
                    MouseArea {
                        id: sma
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                snipMenu.popup()
                            } else {
                                terminal.write(appState.activeTabId, model.command)
                            }
                        }
                    }
                    Menu {
                        id: snipMenu
                        MenuItem {
                            text: "Editar"
                            onTriggered: root.openEditorForCurrentMode(model.snipId)
                        }
                        MenuItem {
                            text: "Excluir"
                            onTriggered: snippets.remove(model.snipId)
                        }
                    }
                }
            }

            // 3 — Chaves
            ListView {
                model: credentials
                clip: true
                spacing: 1
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 44
                    color: cma.containsMouse ? Theme.surfaceHigh : "transparent"
                    Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text { text: model.emoji && model.emoji.length > 0 ? model.emoji : "🔐"; font.pixelSize: 14 }
                        ColumnLayout {
                            spacing: 2; Layout.fillWidth: true
                            Text { text: model.name; color: Theme.fg; font.pixelSize: 13 }
                            Text { text: model.fieldCount + " campos"; color: Theme.fgFaint; font.pixelSize: 11 }
                        }
                    }
                    MouseArea {
                        id: cma
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                credMenu.popup()
                            } else {
                                terminal.write(appState.activeTabId, credentials.renderInline(model.credId))
                            }
                        }
                    }
                    Menu {
                        id: credMenu
                        MenuItem {
                            text: "Editar"
                            onTriggered: root.openEditorForCurrentMode(model.credId)
                        }
                        MenuItem {
                            text: "Excluir"
                            onTriggered: credentials.remove(model.credId)
                        }
                    }
                }
            }
        }
    }
}
