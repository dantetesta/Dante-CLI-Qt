// One tab pill — tinted background+border by `color` prop. Mirrors Swift TabChip.
import QtQuick 6.5
import QtQuick.Controls 6.5
import "."

Rectangle {
    id: chip
    property string tabId
    property string title:  ""
    property color  color:  "#0A84FF"
    property string emoji:  ""
    property bool   pinned: false
    property bool   isActive: false

    signal select()
    signal close()

    width:  Math.min(220, content.implicitWidth + 24)
    height: 36
    radius: 6

    color: Qt.rgba(chip.color.r, chip.color.g, chip.color.b,
                   isActive ? 0.28 : (mouseArea.containsMouse ? 0.16 : 0.10))
    border.color: Qt.rgba(chip.color.r, chip.color.g, chip.color.b, isActive ? 0.65 : 0.20)
    border.width: isActive ? 2 : 1

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        cursorShape: Qt.PointingHandCursor
        onClicked: (mouse) => {
            if (mouse.button === Qt.MiddleButton) chip.close()
            else chip.select()
        }
    }

    Row {
        id: content
        anchors.left:   parent.left
        anchors.right:  closeBtn.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 6
        spacing: 6

        Text {
            text: chip.pinned ? "📌" : (chip.emoji.length > 0 ? chip.emoji : "💻")
            font.pixelSize: 13
        }

        Text {
            text: chip.title
            color: Theme.fg
            font.family: Theme.fontSans
            font.pixelSize: 12.5
            font.weight: chip.isActive ? Font.Bold : Font.Normal
            elide: Text.ElideMiddle
            width: Math.min(implicitWidth, 150)
        }
    }

    Rectangle {
        id: closeBtn
        width: 18; height: 18
        radius: 9
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        color: closeArea.containsMouse ? Theme.danger : "transparent"
        visible: chip.isActive || mouseArea.containsMouse
        Text {
            anchors.centerIn: parent
            text: "×"
            color: closeArea.containsMouse ? "white" : Theme.fgMuted
            font.pixelSize: 13
            font.bold: true
        }
        MouseArea {
            id: closeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: chip.close()
        }
    }
}
