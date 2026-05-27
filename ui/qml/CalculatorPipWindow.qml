// SPEC-172 — Picture-in-Picture calculator window.
// Detaches CalculatorView into its own borderless, always-on-top Window so
// the user can keep the calculator visible while working in another tab.
// Sharing the `calculator` context property means memory / history / display
// stay in sync between the in-tab view and the PiP — they are two views on
// the same C++ controller.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Window 6.5
import QtQuick.Layouts 6.5
import "."

Window {
    id: pip

    /// Caller can listen for this to update tray state / restore the in-tab
    /// view when the PiP is dismissed.
    signal closed()

    width: 340
    height: 520
    minimumWidth: 280
    minimumHeight: 440
    color: "transparent"

    // Borderless + always-on-top. WindowStaysOnTopHint keeps it pinned above
    // the main app even when focus is elsewhere.
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    title: qsTr("Calculadora — PiP")

    // Body. Rounded surface drawn manually because the window is frameless.
    Rectangle {
        id: shell
        anchors.fill: parent
        color: Theme.ink
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // ─── Drag header ─── grabs the whole window.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: Theme.surface
                radius: Theme.radiusLg

                // Bottom border line.
                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                    height: 1; color: Theme.borderSoft
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 4
                    spacing: 6

                    Text {
                        text: "🧮"
                        font.pixelSize: 13
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Calculadora")
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    // Close.
                    Button {
                        flat: true
                        text: "×"
                        implicitWidth: 24
                        implicitHeight: 24
                        onClicked: { pip.closed(); pip.close() }
                        background: Rectangle {
                            color: parent.hovered ? Theme.danger : "transparent"
                            radius: 4
                            Behavior on color { ColorAnimation { duration: 140 } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? "white" : Theme.fgMuted
                            font.pixelSize: 14
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 28
                    cursorShape: Qt.SizeAllCursor
                    property real startX
                    property real startY
                    property int  startWinX
                    property int  startWinY
                    onPressed: (mouse) => {
                        startX = mouse.x
                        startY = mouse.y
                        startWinX = pip.x
                        startWinY = pip.y
                    }
                    onPositionChanged: (mouse) => {
                        if (!pressed) return
                        pip.x = startWinX + (mouse.x - startX)
                        pip.y = startWinY + (mouse.y - startY)
                    }
                }
            }

            // ─── Calculator body. Shares the `calculator` context property
            // with the in-tab view, so memory + display stay synced. ───
            CalculatorView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Compact mode in PiP: history off by default to save real
                // estate. User can still toggle it from the header.
                historyOpen: false
            }
        }

        // Resize grip — bottom right, identical idiom to MovablePopup.
        Item {
            width: 18
            height: 18
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            Repeater {
                model: 3
                delegate: Rectangle {
                    width: 8
                    height: 1
                    color: Theme.fgFaint
                    rotation: -45
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 3 + index * 3
                    anchors.bottomMargin: 3 + index * 3
                }
            }

            MouseArea {
                anchors.fill: parent
                anchors.margins: -6
                cursorShape: Qt.SizeFDiagCursor
                property real startX
                property real startY
                property int  startW
                property int  startH
                onPressed: (mouse) => {
                    startX = mouse.x
                    startY = mouse.y
                    startW = pip.width
                    startH = pip.height
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    pip.width  = Math.max(pip.minimumWidth,  startW + (mouse.x - startX))
                    pip.height = Math.max(pip.minimumHeight, startH + (mouse.y - startY))
                }
            }
        }
    }

    // Esc closes the PiP — matches every other modal's affordance.
    Shortcut {
        sequence: "Esc"
        onActivated: { pip.closed(); pip.close() }
    }

    Component.onCompleted: {
        // Center on the primary screen when first shown.
        if (Screen.virtualX !== undefined) {
            pip.x = Screen.virtualX + (Screen.width  - pip.width)  / 2
            pip.y = Screen.virtualY + (Screen.height - pip.height) / 2
        }
        pip.visible = true
        pip.raise()
        pip.requestActivate()
    }
}
