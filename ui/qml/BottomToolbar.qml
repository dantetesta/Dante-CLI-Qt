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

        ToolbarButton { iconText: "✨"; label: "AI" }
        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16; Rectangle { anchors.fill: parent; color: Theme.borderSoft } }

        ToolbarButton { iconText: "🎤"; label: "Voz" }
        ToolbarButton { iconText: "💡"; label: "Explicar" }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16; Rectangle { anchors.fill: parent; color: Theme.borderSoft } }
        ToolbarButton { iconText: "▦"; label: "Layout" }

        Item { Layout.preferredWidth: 1; Layout.preferredHeight: 16; Rectangle { anchors.fill: parent; color: Theme.borderSoft } }
        Text { text: "0% · 0 B"; color: Theme.fgFaint; font.family: Theme.fontMono; font.pixelSize: 11 }

        Item { Layout.fillWidth: true }

        ToolbarButton { iconText: "❒"; label: ""; onClicked: appState.rightSidebarVisible = !appState.rightSidebarVisible }
        ToolbarButton { iconText: "⚙"; label: "" }
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
