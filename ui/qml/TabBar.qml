// Horizontal scrollable tab strip — list view bound to TabsModel.
//
// Polish: a 2px accent-colored underline slides horizontally along the
// bottom of the strip and anchors to whichever chip is active. The slide
// is driven by Behaviors on x/width so it animates whether you click a
// tab, close one, or reorder them in the future.
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

            // Track the active chip so we can park a sliding underline
            // beneath it. Updated by each delegate that becomes active.
            property real activeChipX: 0
            property real activeChipW: 0
            property bool hasActive: false

            delegate: TabChip {
                tabId:    model.tabId
                title:    model.title
                tint:     model.color
                emoji:    model.emoji
                pinned:   model.pinned
                isActive: model.active
                onSelect: appState.selectTab(model.tabId)
                onClose:  appState.closeTab(model.tabId)

                // Publish position upward when active.
                onIsActiveChanged: if (isActive) publishGeometry()
                onXChanged:        if (isActive) publishGeometry()
                onWidthChanged:    if (isActive) publishGeometry()
                Component.onCompleted: if (isActive) publishGeometry()

                function publishGeometry() {
                    list.activeChipX = x
                    list.activeChipW = width
                    list.hasActive = true
                }
            }

            // Sliding underline indicator. Positioned in list-content
            // coordinates so it scrolls with the tab strip.
            Rectangle {
                id: underline
                visible: list.hasActive && list.count > 0
                z: 2
                height: 2
                radius: 1
                color: Theme.accent
                opacity: 0.85
                y: list.height - height
                x: list.activeChipX
                width: list.activeChipW

                Behavior on x     { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: 160 } }
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
            background: Rectangle {
                color: parent.hovered ? Theme.surfaceHigh : "transparent"
                radius: Theme.radiusSm
                Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
            }
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
