// One empty grid slot — the placeholder that fills any cell of a multi-pane
// workspace until the user assigns a tab to it. Mirrors the Swift sibling's
// "Slot vazio" cell:
//   • centered stacked-rectangles icon + "Slot vazio" label
//   • subtitle hint
//   • row of 4 tinted shortcut buttons: Terminal / Navegador / Anônimo / Vídeo
// The buttons emit `requested(kind)` — the host (GridWorkspace) decides what
// "kind" means (spawn a terminal, open a browser, etc.).
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.ink
    border.color: Theme.borderSoft
    border.width: 1
    radius: Theme.radiusSm

    /// Emitted when the user clicks one of the four shortcuts. Possible
    /// values: "terminal" | "browser" | "anonymous" | "video".
    signal requested(string kind)

    /// SPEC-170: identity of the host tab + this cell. GridWorkspace sets
    /// both so the drop handler can persist the assignment via AppState.
    property string hostTabId: ""
    property int    cellIndex: -1
    /// Emitted after a successful drop so the host can refresh / animate in
    /// the embedded content.
    signal tabDropped(string sourceTabId, int cellIndex)

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 10

        // Stacked-rectangles glyph (matches the Swift slot.placeholder icon).
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 42
            Layout.preferredHeight: 36
            Rectangle {
                width: 30; height: 22; radius: 4
                color: "transparent"
                border.color: Theme.fgFaint
                border.width: 1.4
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Rectangle {
                width: 24; height: 8; radius: 3
                color: "transparent"
                border.color: Theme.fgFaint
                border.width: 1.4
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Slot vazio")
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 13
            font.weight: Font.Medium
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Arraste uma aba aqui ou escolha um tipo abaixo")
            color: Theme.fgFaint
            font.family: Theme.fontSans
            font.pixelSize: 11
        }

        // Shortcut buttons row.
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6
            spacing: 10
            ShortcutBtn { glyph: "[+]";  label: qsTr("Terminal");  tint: "#22C55E"; onClicked: root.requested("terminal")  }
            ShortcutBtn { glyph: "🌐";   label: qsTr("Navegador"); tint: "#3B82F6"; onClicked: root.requested("browser")   }
            ShortcutBtn { glyph: "🎭";   label: qsTr("Anônimo");   tint: "#8B5CF6"; onClicked: root.requested("anonymous") }
            ShortcutBtn { glyph: "▶";    label: qsTr("Vídeo");     tint: "#DC2626"; onClicked: root.requested("video")     }
        }
    }

    /* ─── DropArea (SPEC-170) — wires the chip→slot drop to AppState.
       The dragged TabChip exposes its tabId via the "dante.tab" key + a
       custom "application/x-dante-tab-id" MIME entry. On drop we persist
       the assignment and emit `tabDropped` so the host can swap the
       EmptySlot for the assigned tab's content. ─── */
    DropArea {
        anchors.fill: parent
        keys: ["dante.tab"]
        onEntered: (drag) => {
            dropOverlay.opacity = 1
            pulseAnim.restart()
        }
        onExited: {
            dropOverlay.opacity = 0
            pulseAnim.stop()
        }
        onDropped: (drop) => {
            dropOverlay.opacity = 0
            pulseAnim.stop()
            const sourceId = drop.getDataAsString("application/x-dante-tab-id")
                          || drop.getDataAsString("text/plain")
            if (sourceId && sourceId.length > 0 && root.hostTabId.length > 0
                && root.cellIndex >= 0) {
                appState.attachTabToSlot(sourceId, root.hostTabId, root.cellIndex)
                root.tabDropped(sourceId, root.cellIndex)
                drop.accept(Qt.MoveAction)
            } else {
                drop.accepted = false
            }
        }
    }
    Rectangle {
        id: dropOverlay
        anchors.fill: parent
        anchors.margins: 4
        radius: Theme.radiusSm
        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
        border.color: Theme.accent
        border.width: 2
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }

        // Pulse the accent border while a drag is hovering (mirrors the
        // Swift sibling's "looking-at-a-drop-target" affordance).
        SequentialAnimation on border.width {
            id: pulseAnim
            running: false
            loops: Animation.Infinite
            NumberAnimation { from: 2; to: 3; duration: 380; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 3; to: 2; duration: 380; easing.type: Easing.InOutQuad }
        }

        Text {
            anchors.centerIn: parent
            text: qsTr("Soltar para preencher o slot")
            color: Theme.accent
            font.family: Theme.fontSans
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
    }

    /* ─── Local component ─── */
    component ShortcutBtn: Rectangle {
        property string glyph
        property string label
        property color  tint
        signal clicked()
        Layout.preferredWidth: 64
        Layout.preferredHeight: 58
        radius: 8
        // Subtle tinted background, full-color border + label.
        color: btnMa.containsMouse
            ? Qt.rgba(tint.r, tint.g, tint.b, 0.22)
            : Qt.rgba(tint.r, tint.g, tint.b, 0.10)
        border.color: tint
        border.width: 1
        scale: btnMa.pressed ? 0.95 : 1.0
        transformOrigin: Item.Center
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
        Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutQuad } }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: glyph
                color: tint
                font.pixelSize: 18
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: label
                color: tint
                font.family: Theme.fontSans
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }
        MouseArea {
            id: btnMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
