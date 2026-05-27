// Shared row delegate for Skills / Agents / MCPs lists.
//
// Click anywhere on the row -> emit `clicked()`. Trailing area hosts
// the scope chip ("USER" / "PROJ") and an action button ("Inserir").
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: row

    // Required from the model.
    property string itemName: ""
    property string itemDescription: ""
    property string itemScope: "user"
    property string actionLabel: qsTr("Inserir")

    signal clicked()
    signal actionTriggered()

    height: layout.implicitHeight + 12
    color: hoverArea.containsMouse ? Theme.surfaceHigh : "transparent"
    radius: Theme.radiusSm
    border.width: 0

    Behavior on color {
        ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: row.clicked()
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 8
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 6
            Layout.preferredHeight: 6
            radius: 3
            color: row.itemScope === "project" ? Theme.warn : Theme.accent
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 6
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: row.itemName
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Rectangle {
                    visible: row.itemScope === "project"
                    radius: 7
                    color: Qt.rgba(0.96, 0.62, 0.04, 0.18)
                    border.color: Qt.rgba(0.96, 0.62, 0.04, 0.45)
                    border.width: 1
                    Layout.preferredHeight: 14
                    Layout.preferredWidth: chipText.implicitWidth + 10
                    Text {
                        id: chipText
                        anchors.centerIn: parent
                        text: "PROJ"
                        color: Theme.warn
                        font.family: Theme.fontSans
                        font.pixelSize: 8
                        font.weight: Font.Bold
                    }
                }
            }

            Text {
                visible: row.itemDescription.length > 0
                text: row.itemDescription
                color: Theme.fgMuted
                font.family: Theme.fontSans
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
                Layout.fillWidth: true
            }
        }

        Rectangle {
            id: actionBtn
            Layout.preferredHeight: 22
            Layout.preferredWidth: actionLabelText.implicitWidth + 14
            radius: Theme.radiusSm
            color: actionArea.containsMouse ? Theme.accent : Theme.accentDim
            border.color: Theme.accentSoft
            border.width: 1
            opacity: hoverArea.containsMouse || actionArea.containsMouse ? 1.0 : 0.0
            Behavior on opacity {
                NumberAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing }
            }
            Behavior on color {
                ColorAnimation { duration: Theme.motionFast; easing.type: Theme.motionEasing }
            }
            Text {
                id: actionLabelText
                anchors.centerIn: parent
                text: row.actionLabel
                color: actionArea.containsMouse ? Theme.fgStrong : Theme.accent
                font.family: Theme.fontSans
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
            MouseArea {
                id: actionArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: row.actionTriggered()
            }
        }
    }
}
