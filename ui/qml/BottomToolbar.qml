// Bottom action bar — AI launcher, mic, explain, layout, stats, settings.
// Stub buttons that fire signals; wire up later.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    height: 42
    color: Qt.rgba(0.094, 0.094, 0.110, 0.95) // surface w/ slight transparency

    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 1; color: Theme.borderSoft
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 6

        ToolbarButton { iconText: "✨"; label: "AI"; onClicked: ai.toggle() }
        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16; Rectangle { anchors.fill: parent; color: Theme.borderSoft } }

        ToolbarButton {
            iconText: "🎤"
            // Label swaps to "Parar" while a take is in flight so the
            // affordance for the toggle is unambiguous.
            label: (typeof voice !== "undefined" && voice && voice.recording) ? "Parar" : "Voz"
            onClicked: {
                if (typeof voice === "undefined" || !voice) return
                voice.toggleRecording()
            }
        }
        ToolbarButton { iconText: "💡"; label: "Explicar"; onClicked: ai.show() }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16; Rectangle { anchors.fill: parent; color: Theme.borderSoft } }
        ToolbarButton {
            id: layoutBtn
            iconText: "▦"
            label: "Layout"
            onClicked: layoutMenu.open()

            // Tiny chooser popup: split vertical / horizontal / none.
            // Sits above the toolbar (negative y) to avoid being clipped
            // against the bottom of the window.
            Popup {
                id: layoutMenu
                y: -implicitHeight - 6
                x: 0
                padding: 6
                background: Rectangle {
                    color: Theme.surfaceHigh
                    border.color: Theme.border
                    border.width: 1
                    radius: Theme.radiusSm
                }
                contentItem: ColumnLayout {
                    spacing: 2
                    LayoutMenuItem {
                        text: "▮▮  Split vertical"
                        shortcutHint: "Ctrl+\\"
                        onTriggered: { appState.splitActive("vertical");   layoutMenu.close() }
                    }
                    LayoutMenuItem {
                        text: "▬▬  Split horizontal"
                        shortcutHint: "Ctrl+Shift+\\"
                        onTriggered: { appState.splitActive("horizontal"); layoutMenu.close() }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.borderSoft
                    }
                    LayoutMenuItem {
                        text: "▢   Sem split"
                        shortcutHint: ""
                        enabled: appState.tabSplitMode(appState.activeTabId) !== ""
                        onTriggered: {
                            // Kill the b-pane's PTY before clearing state.
                            const sb = appState.tabSecondSessionId(appState.activeTabId)
                            if (sb) terminal.kill(sb)
                            appState.splitActive("")
                            layoutMenu.close()
                        }
                    }
                }
            }
        }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16; Rectangle { anchors.fill: parent; color: Theme.borderSoft } }
        Text { text: "0% · 0 B"; color: Theme.fgFaint; font.family: Theme.fontMono; font.pixelSize: 11 }

        Item { Layout.fillWidth: true }

        ToolbarButton { iconText: "❒"; label: ""; onClicked: appState.rightSidebarVisible = !appState.rightSidebarVisible }
        ToolbarButton { iconText: "⚙"; label: "" }
    }

    // Small clickable row used in the Layout popup. Disabled rows mute
    // their text color and ignore clicks (same convention as a native menu).
    component LayoutMenuItem: Rectangle {
        id: lmi
        property string text: ""
        property string shortcutHint: ""
        property bool   enabled: true
        signal triggered()

        implicitWidth:  220
        implicitHeight: 28
        radius: 4
        color: rowMa.containsMouse && lmi.enabled ? Theme.surfaceTop : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 12
            Text {
                text: lmi.text
                color: lmi.enabled ? Theme.fg : Theme.fgDim
                font.pixelSize: 12
                Layout.fillWidth: true
            }
            Text {
                text: lmi.shortcutHint
                color: Theme.fgFaint
                font.pixelSize: 10
                font.family: Theme.fontMono
                visible: lmi.shortcutHint.length > 0
            }
        }
        MouseArea {
            id: rowMa
            anchors.fill: parent
            hoverEnabled: lmi.enabled
            cursorShape: lmi.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: { if (lmi.enabled) lmi.triggered() }
        }
    }

    component ToolbarButton: Rectangle {
        property string iconText: ""
        property string label: ""
        signal clicked()

        Layout.preferredHeight: 28
        Layout.preferredWidth:  label.length > 0 ? content.implicitWidth + 18 : 30
        radius: Theme.radiusSm
        color: ma.containsMouse ? Theme.surfaceHigh : "transparent"
        border.color: ma.containsMouse ? Theme.border : "transparent"
        border.width: 1

        Row {
            id: content
            anchors.centerIn: parent
            spacing: 6
            Text { text: iconText; color: Theme.fg; font.pixelSize: 13 }
            Text { text: label; color: Theme.fg; font.pixelSize: 12; visible: label.length > 0 }
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
