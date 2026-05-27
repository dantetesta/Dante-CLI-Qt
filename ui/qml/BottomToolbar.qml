// Bottom action bar — AI launcher, mic, explain, layout, stats, settings.
// Layout + Settings buttons delegate to the modal popups mounted at the root
// of Main.qml (id-lookup walks up the QML scope chain).
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

        ToolbarButton { iconText: "✨"; label: qsTr("AI"); onClicked: ai.toggle() }
        Sep {}

        ToolbarButton {
            iconText: "🎤"
            // Label swaps to "Parar" while a take is in flight so the
            // affordance for the toggle is unambiguous.
            label: (typeof voice !== "undefined" && voice && voice.recording) ? qsTr("Parar") : qsTr("Voz")
            onClicked: {
                if (typeof voice === "undefined" || !voice) return
                voice.toggleRecording()
            }
        }
        ToolbarButton { iconText: "💡"; label: qsTr("Explicar"); onClicked: ai.show() }

        Sep {}
        ToolbarButton {
            iconText: "▦"
            label: qsTr("Layout do split")
            onClicked: layoutDesigner.open()
        }

        Sep {}
        // Live CPU/RAM pill wired to dante::ProcessStatsController. Muted so it
        // reads as ambient telemetry, not an action. Tooltip on hover.
        Rectangle {
            id: statsPill
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 22
            Layout.preferredWidth: statsRow.implicitWidth + 14
            radius: Theme.radiusSm
            color: statsHover.containsMouse ? Theme.surfaceHigh : "transparent"
            border.color: statsHover.containsMouse ? Theme.borderSoft : "transparent"
            border.width: 1
            Behavior on color       { ColorAnimation { duration: Theme.motionFast } }
            Behavior on border.color{ ColorAnimation { duration: Theme.motionFast } }

            Row {
                id: statsRow
                anchors.centerIn: parent
                spacing: 6
                Text {
                    text: (typeof processStats !== "undefined" && processStats)
                          ? ("CPU " + Math.round(processStats.cpuPercent) + "%")
                          : "CPU 0%"
                    color: Theme.fgMuted
                    font.family: Theme.fontMono
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "·"
                    color: Theme.fgFaint
                    font.family: Theme.fontMono
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: (typeof processStats !== "undefined" && processStats)
                          ? (Math.round(processStats.memoryMB) + " MB")
                          : "0 MB"
                    color: Theme.fgMuted
                    font.family: Theme.fontMono
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: statsHover
                anchors.fill: parent
                hoverEnabled: true
                ToolTip.text: (typeof processStats !== "undefined" && processStats)
                              ? processStats.tooltipText
                              : ""
                ToolTip.visible: containsMouse
                                 && (typeof processStats !== "undefined" && processStats)
                ToolTip.delay: 400
            }
        }

        Item { Layout.fillWidth: true }

        // Right-side cluster — sidebar toggle + settings, both labeled now
        // (previously they were unmoored emoji glyphs that looked like
        // decoration, not buttons).
        ToolbarButton {
            iconText: "❒"
            label: qsTr("Painel")
            tooltip: qsTr("Mostrar/Esconder painel direito")
            onClicked: appState.rightSidebarVisible = !appState.rightSidebarVisible
        }
        ToolbarButton {
            iconText: "⚙"
            label: qsTr("Ajustes")
            tooltip: qsTr("Configurações")
            onClicked: settingsPanel.open()
        }
    }

    component Sep: Item {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 16
        Rectangle { anchors.fill: parent; color: Theme.borderSoft }
    }

    component ToolbarButton: Rectangle {
        property string iconText: ""
        property string label: ""
        property string tooltip: ""
        signal clicked()

        Layout.preferredHeight: 30
        Layout.preferredWidth:  label.length > 0 ? content.implicitWidth + 22 : 36
        radius: Theme.radiusSm
        color: ma.containsMouse ? Theme.surfaceHigh : "transparent"
        border.color: ma.containsMouse ? Theme.border : "transparent"
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }

        Row {
            id: content
            anchors.centerIn: parent
            spacing: 6
            Text { text: iconText; color: Theme.fg; font.pixelSize: 14 }
            Text {
                text: label
                color: Theme.fg
                font.pixelSize: 12
                font.family: Theme.fontSans
                visible: label.length > 0
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
            ToolTip.text: parent.tooltip
            ToolTip.visible: containsMouse && parent.tooltip.length > 0
            ToolTip.delay: 400
        }
    }
}
