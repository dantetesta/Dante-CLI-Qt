// Slim banner that slides down from the top whenever updater.available
// becomes true. Two buttons: "Atualizar" (downloadAndInstall) and "Depois".
// While installing it morphs into a progress bar with cancel-disabled.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Rectangle {
    id: root
    height: 44
    color: Theme.accentDim
    border.color: Theme.accentSoft
    border.width: 1

    // Slide in/out. When neither available nor installing, height collapses
    // (Main.qml has Behavior on Layout.preferredHeight bound to root.height).
    visible: !!(updater && (updater.available || updater.installing))
    opacity: visible ? 1.0 : 0.0
    Behavior on opacity { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 8
        spacing: 12

        // Mini icon + label.
        Text {
            text: "⬆"
            color: Theme.accent
            font.pixelSize: 16
            font.bold: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                text: updater && updater.installing
                    ? qsTr("Instalando atualização… %1%").arg(updater.progress)
                    : updater && updater.info
                        ? qsTr("Nova versão %1 disponível").arg(updater.info.version || "")
                        : ""
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 13
                font.weight: Font.Medium
                elide: Text.ElideRight
            }
            Text {
                visible: !(updater && updater.installing) && text.length > 0
                text: updater && updater.info ? (updater.info.notes || "") : ""
                color: Theme.fgMuted
                font.family: Theme.fontSans
                font.pixelSize: 11
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        // Progress bar (visible only while installing).
        Item {
            visible: updater && updater.installing
            Layout.preferredWidth: 160
            Layout.preferredHeight: 6
            Rectangle {
                anchors.fill: parent
                radius: 3
                color: Theme.surfaceHigh
            }
            Rectangle {
                radius: 3
                color: Theme.accent
                height: parent.height
                width: parent.width * (updater && updater.progress ? updater.progress / 100 : 0)
                Behavior on width { NumberAnimation { duration: 120 } }
            }
        }

        Button {
            text: qsTr("Atualizar")
            visible: updater && updater.available && !updater.installing
            onClicked: updater.downloadAndInstall()
            background: Rectangle {
                radius: 6
                color: parent.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }
            contentItem: Text {
                text: parent.text
                color: "#FFFFFF"
                font.family: Theme.fontSans
                font.pixelSize: 12
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:   Text.AlignVCenter
            }
        }

        Button {
            text: qsTr("Depois")
            flat: true
            visible: updater && updater.available && !updater.installing
            onClicked: updater.dismiss()
            background: Rectangle {
                radius: 6
                color: parent.hovered ? Theme.surfaceHigh : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }
            contentItem: Text {
                text: parent.text
                color: Theme.fg
                font.family: Theme.fontSans
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:   Text.AlignVCenter
            }
        }
    }
}
