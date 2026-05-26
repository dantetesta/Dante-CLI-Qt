// Horizontal scrollable tab strip — list view bound to TabsModel.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    height: 40
    color:  Theme.ink

    Rectangle {                          // bottom divider
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color:  Theme.borderSoft
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin:  8
        anchors.rightMargin: 4
        anchors.bottomMargin: 0
        spacing: 6

        ListView {
            id: list
            Layout.fillWidth:  true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 6
            interactive: true
            clip: true
            model: tabs

            delegate: TabChip {
                tabId:    model.tabId
                title:    model.title
                color:    model.color
                emoji:    model.emoji
                pinned:   model.pinned
                isActive: model.active
                onSelect: appState.selectTab(model.tabId)
                onClose:  appState.closeTab(model.tabId)
            }
        }

        Button {
            text: "+"
            flat: true
            implicitWidth: 32
            implicitHeight: 28
            ToolTip.text: "Nova aba (⌘T)"
            ToolTip.visible: hovered
            onClicked: appState.addTab("Terminal")
            background: Rectangle { color: parent.hovered ? Theme.surfaceHigh : "transparent"; radius: Theme.radiusSm }
            contentItem: Text {
                text: parent.text
                color: Theme.accent
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:   Text.AlignVCenter
            }
        }
    }
}
