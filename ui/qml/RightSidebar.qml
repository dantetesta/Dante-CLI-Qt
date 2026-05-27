// Right-side dock — three tabs (Skills / Agents / MCPs) sourced from
// `~/.claude/` via the `resources` ResourcesController.
//
// Toggled by Main.qml via `appState.rightSidebarVisible` (Ctrl+]). The
// container itself is unconditionally mounted; the parent controls
// preferred width to animate the slide-in/out.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    color: Theme.surface
    clip: true

    property int selectedTab: 0
    property string query: ""

    // Left edge divider so the dock visually separates from the
    // main column without a heavy shadow.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Theme.borderSoft
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 1
        spacing: 0

        // ───── Header: tab strip + refresh ─────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 10
                anchors.bottomMargin: 6
                spacing: 6

                Repeater {
                    model: [
                        { label: qsTr("Skills"),  idx: 0 },
                        { label: qsTr("Agents"),  idx: 1 },
                        { label: qsTr("MCPs"),    idx: 2 },
                    ]
                    Rectangle {
                        id: tabBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        readonly property bool selected: root.selectedTab === modelData.idx
                        color: selected ? Theme.accentDim
                                        : (tabArea.containsMouse ? Theme.surfaceHigh : "transparent")
                        border.color: selected ? Theme.accentSoft : Theme.borderSoft
                        border.width: 1
                        radius: Theme.radiusSm
                        Behavior on color {
                            ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing }
                        }
                        Behavior on border.color {
                            ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: tabBtn.selected ? Theme.accent : Theme.fgMuted
                            font.family: Theme.fontSans
                            font.pixelSize: 11
                            font.weight: tabBtn.selected ? Font.DemiBold : Font.Medium
                        }
                        MouseArea {
                            id: tabArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectedTab = modelData.idx
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    radius: Theme.radiusSm
                    color: refreshArea.containsMouse ? Theme.surfaceHigh : "transparent"
                    border.color: Theme.borderSoft
                    border.width: 1
                    Behavior on color {
                        ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing }
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "↻"
                        color: Theme.fgMuted
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: refreshArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: resources.refresh()
                        ToolTip.text: qsTr("Recarregar")
                        ToolTip.visible: containsMouse
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSoft }

        // ───── Search ─────
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 8
            Layout.bottomMargin: 6
            Layout.preferredHeight: 30
            color: Theme.surfaceHigh
            border.color: Theme.border
            border.width: 1
            radius: Theme.radiusMd

            TextField {
                id: searchField
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                placeholderText: qsTr("Buscar…")
                placeholderTextColor: Theme.fgFaint
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 12
                background: null
                onTextChanged: root.query = text
            }
        }

        // ───── Body ─────
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 4
            Layout.rightMargin: 4
            Layout.bottomMargin: 6
            currentIndex: root.selectedTab

            SkillsTab { query: root.query }
            AgentsTab { query: root.query }
            MCPsTab   { query: root.query }
        }

        // ───── Footer scan indicator ─────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            color: Theme.surfaceLow
            visible: resources && resources.scanning
            Text {
                anchors.centerIn: parent
                text: qsTr("Lendo ~/.claude/…")
                color: Theme.fgFaint
                font.family: Theme.fontSans
                font.pixelSize: 10
            }
        }
    }
}
