// Reusable Popup with a draggable title bar + a resize grip in the bottom-
// right. Editors / dialogs subclass this with:
//
//   MovablePopup {
//       title: qsTr("Meu editor")
//       contentItem: ColumnLayout { … }     // the form fields
//   }
//
// Sized like a normal Popup; parent defaults to the window's Overlay so
// `anchors.centerIn` centers on the WINDOW (not on the caller's local item).
// That fixes the bug where editors mounted inside Sidebar.qml would render
// off to one side because their parent was the sidebar, not the root.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

Popup {
    id: root
    modal: true
    focus: true
    padding: 0

    // Always parented to the window's overlay so the popup floats above the
    // whole UI no matter where it's instantiated. Caller can still override
    // by assigning parent: someItem.
    parent: Overlay.overlay
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 140; easing.type: Easing.OutCubic } }

    /// Title shown in the drag header.
    property string title: ""
    /// Optional emoji/icon before the title.
    property string icon: ""
    /// Min size when the user drags the grip.
    property int    minWidth: 320
    property int    minHeight: 240
    property bool   showCloseButton: true

    /// Everything declared directly inside MovablePopup lands here. We use
    /// `data` (not `children`) so non-visual objects like FileDialog can
    /// coexist with the layout's QQuickItem children.
    default property alias contentArea: bodyArea.data

    background: Rectangle {
        color: Theme.surfaceHigh
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
    }

    /// The Popup's contentItem replaces our background's children area.
    /// We use a ColumnLayout that pins the drag header to the top and the
    /// resize grip to the bottom-right corner of the body.
    contentItem: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            /* ─── Drag header ─── */
            Rectangle {
                id: header
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: Theme.surface
                topLeftRadius: Theme.radiusLg
                topRightRadius: Theme.radiusLg

                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                    height: 1; color: Theme.borderSoft
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 6
                    spacing: 8

                    Text {
                        text: root.icon.length > 0 ? root.icon : ""
                        font.pixelSize: 13
                        visible: root.icon.length > 0
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.title
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Button {
                        visible: root.showCloseButton
                        flat: true
                        text: "×"
                        implicitWidth: 26
                        implicitHeight: 26
                        onClicked: root.close()
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceTop : "transparent"
                            radius: 4
                            Behavior on color { ColorAnimation { duration: 140 } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: Theme.fgMuted
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // Drag the whole popup by the header.
                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 30   // leave room for the × button
                    cursorShape: Qt.SizeAllCursor
                    property real startX
                    property real startY
                    property real startPopupX
                    property real startPopupY
                    onPressed: (mouse) => {
                        startX = mouse.x
                        startY = mouse.y
                        startPopupX = root.x
                        startPopupY = root.y
                    }
                    onPositionChanged: (mouse) => {
                        if (!pressed) return
                        // Translate the click delta to popup x/y.
                        const dx = mouse.x - startX
                        const dy = mouse.y - startY
                        const newX = startPopupX + dx
                        const newY = startPopupY + dy
                        // Clamp inside the overlay so we don't drag off-screen.
                        const o = root.parent
                        root.x = Math.max(0, Math.min(o.width  - root.width,  newX))
                        root.y = Math.max(0, Math.min(o.height - root.height, newY))
                    }
                }
            }

            /* ─── Body where the user's contentArea lives ─── */
            Item {
                id: bodyArea
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        /* ─── Resize grip (bottom-right) ─── */
        Item {
            width: 18
            height: 18
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            // Visual: 3 diagonal strokes — same convention as native widgets.
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
                anchors.margins: -6  // bigger hit area
                cursorShape: Qt.SizeFDiagCursor
                property real startX
                property real startY
                property real startW
                property real startH
                onPressed: (mouse) => {
                    startX = mouse.x
                    startY = mouse.y
                    startW = root.width
                    startH = root.height
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    const dw = mouse.x - startX
                    const dh = mouse.y - startY
                    root.width  = Math.max(root.minWidth,  startW + dw)
                    root.height = Math.max(root.minHeight, startH + dh)
                }
            }
        }
    }
}
