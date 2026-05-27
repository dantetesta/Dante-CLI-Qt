// Skills tab inside the RightSidebar. Lists skills from
// `~/.claude/skills/<name>/SKILL.md` (and the project equivalent).
//
// Filter mirrors the parent's `query` string so all three tabs share a
// single search field.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: tab
    property string query: ""

    function matches(name, desc, q) {
        if (!q) return true
        const needle = q.toLowerCase()
        return name.toLowerCase().indexOf(needle) >= 0
            || (desc && desc.toLowerCase().indexOf(needle) >= 0)
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        spacing: 2
        model: resources.skillsModel
        boundsBehavior: Flickable.StopAtBounds

        delegate: ResourceRow {
            width: ListView.view.width
            itemName: model.name || ""
            itemDescription: model.description || ""
            itemScope: model.scope || "user"
            actionLabel: qsTr("Inserir")
            visible: tab.matches(itemName, itemDescription, tab.query)
            height: visible ? implicitHeight : 0
            onClicked: resources.invokeSkill(itemName)
            onActionTriggered: resources.invokeSkill(itemName)
        }

        Text {
            anchors.centerIn: parent
            visible: listView.count === 0
            text: qsTr("Nenhuma skill em ~/.claude/skills/")
            color: Theme.fgFaint
            font.family: Theme.fontSans
            font.pixelSize: 11
        }
    }
}
