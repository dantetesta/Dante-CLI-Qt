// SchemePicker — grid of mini preview cards. Each card shows the scheme
// name and a 4-color stripe (background + three accents). Clicking a card
// sets `appState.terminalScheme`, which Theme.qml watches reactively.
//
// Drop this into any container; it autosizes height to fit the grid.
import QtQuick 6.5
import QtQuick.Layouts 6.5
import QtQuick.Controls 6.5
import "."

Item {
    id: root
    implicitHeight: grid.implicitHeight + 24
    implicitWidth:  grid.implicitWidth  + 24

    // Models pulled from the C++ ThemeRegistry context property.
    readonly property var schemeList: schemes ? schemes.list() : []
    readonly property string activeId: appState ? appState.terminalScheme : ""

    GridLayout {
        id: grid
        anchors.fill: parent
        anchors.margins: 12
        columnSpacing: 10
        rowSpacing: 10
        columns: Math.max(1, Math.floor((root.width - 24) / 130))

        Repeater {
            model: root.schemeList
            delegate: Rectangle {
                id: card
                Layout.preferredWidth:  120
                Layout.preferredHeight: 80
                radius: Theme.radiusMd

                readonly property bool isActive: modelData.id === root.activeId
                readonly property var preview: schemes ? schemes.preview(modelData.id) : []

                color: cardArea.containsMouse ? Theme.surfaceHigh : Theme.surface
                border.color: isActive ? Theme.accent
                                       : (cardArea.containsMouse ? Theme.borderStrong : Theme.border)
                border.width: isActive ? 2 : 1
                scale: cardArea.pressed ? 0.97 : 1.0

                Behavior on color        { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing } }
                Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing } }
                Behavior on border.width { NumberAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing } }
                Behavior on scale        { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutQuad } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Text {
                        Layout.fillWidth: true
                        text: modelData.name
                        color: card.isActive ? Theme.fgStrong : Theme.fg
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        font.weight: card.isActive ? Font.DemiBold : Font.Medium
                        elide: Text.ElideRight
                    }

                    // 4-color stripe: background swatch + 3 accents.
                    Row {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 18
                        spacing: 2
                        Repeater {
                            model: card.preview
                            Rectangle {
                                width: (card.width - 8 * 2 - 2 * 3) / 4
                                height: 18
                                radius: 3
                                color: modelData
                                border.color: Qt.rgba(0, 0, 0, 0.25)
                                border.width: 1
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: card.isActive ? "✓ Ativo" : modelData.id
                        color: card.isActive ? Theme.accent : Theme.fgFaint
                        font.family: Theme.fontSans
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: cardArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (appState) appState.terminalScheme = modelData.id
                    }
                    ToolTip.text: modelData.id
                    ToolTip.visible: containsMouse
                    ToolTip.delay: 600
                }
            }
        }
    }
}
