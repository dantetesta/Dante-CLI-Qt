// Agents tab inside the RightSidebar. Lists Claude Code agents
// scanned from `~/.claude/agents/*.md` (and the project equivalent).
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
        model: resources.agentsModel
        boundsBehavior: Flickable.StopAtBounds

        delegate: ResourceRow {
            width: ListView.view.width
            itemName: model.name || ""
            itemDescription: model.description || ""
            itemScope: model.scope || "user"
            actionLabel: qsTr("Inserir")
            visible: tab.matches(itemName, itemDescription, tab.query)
            height: visible ? implicitHeight : 0
            onClicked: resources.invokeAgent(itemName)
            onActionTriggered: resources.invokeAgent(itemName)
        }

        Text {
            anchors.centerIn: parent
            visible: listView.count === 0
            text: qsTr("Nenhum agent em ~/.claude/agents/")
            color: Theme.fgFaint
            font.family: Theme.fontSans
            font.pixelSize: 11
        }
    }
}
