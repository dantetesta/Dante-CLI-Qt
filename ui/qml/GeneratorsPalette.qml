// SPEC-081 — Two-column generators palette.
//
// Left:  category list (built from `generators.categories`).
// Right: cards filtered by selected category + free-text search.
// Picking a card → `autoFill.prepare(templateText)`, which routes through
// AutoFillDialog (placeholders) or directly to the terminal (no placeholders).
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

MovablePopup {
    id: root
    width: 760
    height: 520
    minWidth: 540
    minHeight: 360
    title: qsTr("Geradores de comando")
    icon: "⚙"

    property string selectedCategory: ""
    property string query: ""

    // Whenever the popup opens, reset the selection to the first category
    // and clear the search so the user lands on a clean state.
    onOpened: {
        if (typeof generators !== "undefined" && generators
                && generators.categories.length > 0) {
            selectedCategory = generators.categories[0]
        }
        query = ""
        searchField.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // ─── Search bar ───
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Buscar gerador (git, curl, docker…)")
            font.family: Theme.fontSans
            font.pixelSize: 12
            color: Theme.fg
            text: root.query
            onTextChanged: root.query = text
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // ─── Left: categories ───
            Rectangle {
                Layout.preferredWidth: 160
                Layout.fillHeight: true
                color: Theme.surfaceLow
                border.color: Theme.borderSoft
                radius: Theme.radiusSm

                ListView {
                    id: catList
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    spacing: 2
                    model: (typeof generators !== "undefined" && generators)
                           ? generators.categories : []
                    delegate: Rectangle {
                        width: catList.width
                        height: 30
                        radius: 5
                        color: modelData === root.selectedCategory
                               ? Theme.accentDim
                               : (catMa.containsMouse ? Theme.surfaceTop : "transparent")
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            text: modelData
                            color: modelData === root.selectedCategory
                                   ? Theme.fgStrong : Theme.fg
                            font.family: Theme.fontSans
                            font.pixelSize: 12
                            font.weight: modelData === root.selectedCategory
                                         ? Font.DemiBold : Font.Normal
                        }
                        MouseArea {
                            id: catMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectedCategory = modelData
                        }
                    }
                }
            }

            // ─── Right: cards ───
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.surfaceLow
                border.color: Theme.borderSoft
                radius: Theme.radiusSm

                // We compute the filtered list inline. When the search box is
                // non-empty we ignore the category so the user can jump to any
                // generator by name — feels natural for fuzzy command palette.
                property var rows: {
                    if (typeof generators === "undefined" || !generators) return []
                    const cat = root.query.length > 0 ? "" : root.selectedCategory
                    return generators.filtered(cat, root.query)
                }

                ScrollView {
                    id: cardScroll
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true

                    ColumnLayout {
                        width: cardScroll.availableWidth
                        spacing: 6

                        Repeater {
                            model: cardScroll.parent.rows
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: row.implicitHeight + 16
                                color: cardMa.containsMouse
                                       ? Theme.surfaceTop : Theme.surface
                                border.color: Theme.borderSoft
                                radius: Theme.radiusSm
                                Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                                RowLayout {
                                    id: row
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Text {
                                        text: modelData.icon
                                        font.pixelSize: 18
                                        color: Theme.fg
                                        visible: modelData.icon.length > 0
                                        Layout.preferredWidth: 24
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        Text {
                                            text: modelData.name
                                            color: Theme.fgStrong
                                            font.family: Theme.fontSans
                                            font.pixelSize: 13
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: modelData.description
                                            color: Theme.fgMuted
                                            font.family: Theme.fontSans
                                            font.pixelSize: 11
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Button {
                                        text: qsTr("Usar")
                                        onClicked: root.use(modelData.id)
                                        background: Rectangle {
                                            color: parent.hovered
                                                   ? Theme.accent : Theme.accentSoft
                                            radius: Theme.radiusSm
                                            Behavior on color {
                                                ColorAnimation { duration: Theme.motionFast }
                                            }
                                        }
                                        contentItem: Text {
                                            text: parent.text
                                            color: Theme.fgStrong
                                            font.family: Theme.fontSans
                                            font.pixelSize: 11
                                            font.weight: Font.DemiBold
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }
                                }

                                MouseArea {
                                    id: cardMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    // Don't steal clicks from the "Usar" button.
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                        }

                        Text {
                            visible: cardScroll.parent.rows.length === 0
                            text: qsTr("Nenhum gerador encontrado.")
                            color: Theme.fgFaint
                            font.family: Theme.fontSans
                            font.pixelSize: 12
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 20
                        }
                    }
                }
            }
        }
    }

    // Picks a generator by id, routes the template through AutoFill, and
    // closes the palette so the user lands on the dialog (or the terminal
    // if there are no placeholders).
    function use(id) {
        if (typeof generators === "undefined" || !generators) return
        const tpl = generators.pick(id)
        if (!tpl || tpl.length === 0) return
        close()
        if (typeof autoFill !== "undefined" && autoFill) {
            autoFill.prepare(tpl)
        }
    }
}
