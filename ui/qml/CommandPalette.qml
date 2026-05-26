// Command Palette — Cmd+K / Ctrl+K fuzzy launcher.
//
// Mounted in Main.qml as `CommandPalette { id: palette ... }`. Driven by
// the `palette` context property (PaletteController in C++). Closes on
// Esc, click-outside, or after `execute()`.
//
// Visual spec: centered 600-wide card, Theme.surface w/ 0.95 alpha,
// Theme.borderStrong border, radius Theme.radiusLg. Scale-in + fade.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: root
    anchors.fill: parent
    visible: palette.open || cardScale > 0.96
    z: 200

    // Track our own animated state so we can keep the card mounted while
    // the close animation runs (visible: ...).
    property real cardScale:   palette.open ? 1.0 : 0.95
    property real cardOpacity: palette.open ? 1.0 : 0.0

    Behavior on cardScale {
        NumberAnimation { duration: 180; easing.type: Easing.OutBack; easing.overshoot: 1.3 }
    }
    Behavior on cardOpacity {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    // Scrim — dim the rest of the UI and capture click-outside.
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: (palette && palette.open) ? 0.38 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        MouseArea {
            anchors.fill: parent
            enabled: !!(palette && palette.open)
            onClicked: palette.hide()
        }
    }

    // ────────────────────────── Card ──────────────────────────
    Rectangle {
        id: card
        width: 600
        // Auto-grow up to 480px tall.
        height: Math.min(480, header.height + listWrap.implicitHeight + footer.height + 24)
        x: (parent.width - width) / 2
        y: Math.max(80, (parent.height - height) / 2 - 80) // slightly above center, like Spotlight
        color: Qt.rgba(0.094, 0.094, 0.110, 0.95) // Theme.surface @ 0.95
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
        opacity: root.cardOpacity
        transform: Scale {
            origin.x: card.width / 2
            origin.y: card.height / 2
            xScale: root.cardScale
            yScale: root.cardScale
        }

        // Subtle drop shadow via a soft outline rectangle behind.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            z: -1
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.4)
            border.width: 1
            radius: parent.radius + 1
        }

        // Eat clicks inside the card so the scrim doesn't fire.
        MouseArea { anchors.fill: parent; onClicked: {} }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // ─────── Header / search field ───────
            RowLayout {
                id: header
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "🔍"  // 🔍
                    font.pixelSize: 18
                    color: Theme.fgMuted
                    Layout.alignment: Qt.AlignVCenter
                }

                TextField {
                    id: queryField
                    Layout.fillWidth: true
                    placeholderText: "Digite um comando…"
                    placeholderTextColor: Theme.fgFaint
                    color: Theme.fg
                    font.pixelSize: 16
                    font.family: Theme.fontSans
                    background: Rectangle { color: "transparent"; border.width: 0 }
                    selectByMouse: true
                    text: palette && palette.query !== undefined ? palette.query : ""

                    onTextChanged: if (text !== palette.query) palette.setQuery(text)

                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Down) {
                            palette.selectNext(); event.accepted = true;
                        } else if (event.key === Qt.Key_Up) {
                            palette.selectPrev(); event.accepted = true;
                        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            palette.execute(); event.accepted = true;
                        } else if (event.key === Qt.Key_Escape) {
                            palette.hide(); event.accepted = true;
                        }
                    }

                    Connections {
                        target: palette
                        ignoreUnknownSignals: true
                        function onOpenChanged() {
                            if (palette.open) {
                                queryField.text = ""
                                Qt.callLater(function() { queryField.forceActiveFocus() })
                            }
                        }
                    }
                }
            }

            // Thin separator under the search field.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderSoft
            }

            // ─────── Results list ───────
            Item {
                id: listWrap
                Layout.fillWidth: true
                Layout.fillHeight: true
                implicitHeight: Math.min(360, Math.max(48, results.count * 44 + 8))

                ListView {
                    id: results
                    anchors.fill: parent
                    clip: true
                    model: palette.results
                    spacing: 2
                    interactive: true
                    currentIndex: palette && palette.activeIndex !== undefined ? palette.activeIndex : 0

                    onCountChanged: positionViewAtIndex(palette.activeIndex, ListView.Visible)

                    Connections {
                        target: palette
                        ignoreUnknownSignals: true
                        function onActiveIndexChanged() {
                            results.positionViewAtIndex(palette.activeIndex, ListView.Contain)
                        }
                    }

                    delegate: Rectangle {
                        id: row
                        width: ListView.view.width
                        height: 42
                        radius: Theme.radiusSm
                        color: index === palette.activeIndex
                                   ? Theme.accentDim
                                   : (mouseArea.containsMouse ? Theme.surfaceHigh : "transparent")
                        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 10

                            Text {
                                text: modelData && modelData.icon ? modelData.icon : "•"
                                font.pixelSize: 16
                                color: Theme.fgMuted
                                Layout.preferredWidth: 22
                                horizontalAlignment: Text.AlignHCenter
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    text: modelData ? modelData.label : ""
                                    color: index === palette.activeIndex ? Theme.fgStrong : Theme.fg
                                    font.pixelSize: 14
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData ? modelData.hint : ""
                                    color: Theme.fgFaint
                                    font.pixelSize: 11
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                    visible: text.length > 0
                                    Layout.fillWidth: true
                                }
                            }

                            // Category badge.
                            Rectangle {
                                visible: modelData && modelData.category && modelData.category.length > 0
                                color: Qt.rgba(0.486, 0.510, 1.0, 0.16) // accentSoft-ish
                                border.color: Theme.accentSoft
                                border.width: 1
                                radius: 4
                                implicitWidth: catText.implicitWidth + 12
                                implicitHeight: catText.implicitHeight + 4
                                Text {
                                    id: catText
                                    anchors.centerIn: parent
                                    text: modelData ? modelData.category : ""
                                    color: Theme.fg
                                    font.pixelSize: 10
                                    font.family: Theme.fontSans
                                }
                            }

                            // Shortcut on the right (when present).
                            Text {
                                visible: modelData && modelData.shortcut && modelData.shortcut.length > 0
                                text: modelData ? modelData.shortcut : ""
                                color: Theme.fgFaint
                                font.pixelSize: 11
                                font.family: Theme.fontMono
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: palette.setActiveIndex(index)
                            onClicked: { palette.setActiveIndex(index); palette.execute(); }
                        }
                    }

                    // Empty state.
                    Text {
                        anchors.centerIn: parent
                        visible: results.count === 0
                        text: "Nenhum comando corresponde a «" + palette.query + "»"
                        color: Theme.fgFaint
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                    }
                }
            }

            // ─────── Footer hints ───────
            Rectangle {
                id: footer
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                color: "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "↑↓ navegar  ·  Enter executar  ·  Esc fechar"
                    color: Theme.fgFaint
                    font.pixelSize: 11
                    font.family: Theme.fontSans
                }
            }
        }
    }

    // Esc closes when the field doesn't have focus (e.g. clicked elsewhere).
    Shortcut {
        sequence: "Esc"
        enabled: !!(palette && palette.open)
        onActivated: palette.hide()
    }
}
