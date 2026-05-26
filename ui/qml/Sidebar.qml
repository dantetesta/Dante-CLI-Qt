// 4-mode sidebar: Favoritos / Pastas / Snippets / Chaves.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.surface

    Rectangle {                          // right divider
        anchors.right: parent.right
        anchors.top: parent.top; anchors.bottom: parent.bottom
        width: 1
        color: Theme.borderSoft
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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6
                        color: appState.sidebarMode === modelData.idx ? Theme.accentDim : "transparent"
                        border.color: appState.sidebarMode === modelData.idx ? Theme.accentSoft : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: modelData.glyph
                            font.pixelSize: 14
                            color: appState.sidebarMode === modelData.idx ? Theme.accent : Theme.fgMuted
                        }
                        MouseArea {
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

        /* ───── Search ───── */
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            Layout.leftMargin: 12; Layout.rightMargin: 12
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

        /* ───── Body: per-mode list ───── */
        StackLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
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
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text { text: model.emoji.length > 0 ? model.emoji : "📁"; font.pixelSize: 14 }
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
                        cursorShape: Qt.PointingHandCursor
                        onDoubleClicked: terminal.write(appState.activeTabId, "cd \"" + model.path + "\"\r")
                    }
                }
            }

            // 1 — Pastas (placeholder)
            Text { text: "Files tree (em breve)"; color: Theme.fgFaint; anchors.centerIn: parent; font.pixelSize: 12 }

            // 2 — Snippets
            ListView {
                model: snippets
                clip: true
                spacing: 1
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 44
                    color: sma.containsMouse ? Theme.surfaceHigh : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text { text: model.emoji.length > 0 ? model.emoji : "⚡"; font.pixelSize: 14 }
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
                        cursorShape: Qt.PointingHandCursor
                        onClicked: terminal.write(appState.activeTabId, model.command)
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
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        Text { text: model.emoji.length > 0 ? model.emoji : "🔐"; font.pixelSize: 14 }
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
                        cursorShape: Qt.PointingHandCursor
                        onClicked: terminal.write(appState.activeTabId, credentials.renderInline(model.credId))
                    }
                }
            }
        }
    }
}
