// Settings popup wired to the ⚙ button in BottomToolbar.
// Five sections (matching Swift sibling): Aparência, Terminal, IA, Voz, Sobre.
// Most values bind directly to appState Q_PROPERTYs (writes persist via the
// existing settings.json store).
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Popup {
    id: root

    modal: true
    focus: true
    width: 560
    height: Math.min(640, parent ? parent.height - 80 : 640)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
        color: Theme.surfaceHigh
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
    }

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionStd; easing.type: Easing.OutCubic } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 0

        /* Header */
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 12
                Text {
                    text: "⚙  " + qsTr("Configurações")
                    color: Theme.fgStrong
                    font.family: Theme.fontSans
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "×"
                    flat: true
                    implicitWidth: 34
                    implicitHeight: 34
                    onClicked: root.close()
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: 6
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fgMuted
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: 1; color: Theme.borderSoft
            }
        }

        /* Body — scrollable list of sections */
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: root.width
                spacing: 20

                /* ─── IA ─── */
                Section { title: qsTr("IA — Groq") }
                FieldRow {
                    label: qsTr("API Key")
                    Layout.fillWidth: true
                    TextField {
                        Layout.fillWidth: true
                        text: appState.groqApiKey || ""
                        placeholderText: "gsk_..."
                        echoMode: TextInput.Password
                        onEditingFinished: appState.groqApiKey = text
                        color: Theme.fg
                        background: Rectangle {
                            color: Theme.surface
                            border.color: Theme.border
                            radius: Theme.radiusSm
                        }
                    }
                }

                /* ─── Terminal ─── */
                Section { title: qsTr("Terminal") }

                FieldRow {
                    label: qsTr("Tema")
                    Layout.fillWidth: true
                    ComboBox {
                        Layout.fillWidth: true
                        model: schemes ? schemes.list() : []
                        textRole: "name"
                        valueRole: "id"
                        currentIndex: {
                            if (!schemes) return 0
                            const list = schemes.list()
                            for (let i = 0; i < list.length; ++i)
                                if (list[i].id === appState.terminalScheme) return i
                            return 0
                        }
                        onActivated: (idx) => {
                            const list = schemes.list()
                            if (list[idx]) appState.terminalScheme = list[idx].id
                        }
                    }
                }

                FieldRow {
                    label: qsTr("Fonte")
                    Layout.fillWidth: true
                    TextField {
                        Layout.fillWidth: true
                        text: appState.fontName || ""
                        placeholderText: Theme.fontMono
                        onEditingFinished: appState.fontName = text
                        color: Theme.fg
                        background: Rectangle { color: Theme.surface; border.color: Theme.border; radius: Theme.radiusSm }
                    }
                }

                FieldRow {
                    label: qsTr("Tamanho")
                    Layout.fillWidth: true
                    SpinBox {
                        from: 9
                        to: 28
                        value: appState.fontSize || 13
                        onValueModified: appState.fontSize = value
                    }
                }

                /* ─── Aparência ─── */
                Section { title: qsTr("Aparência") }

                FieldRow {
                    label: qsTr("Sidebar")
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("Largura: %1 px").arg(appState.sidebarWidth)
                        color: Theme.fgMuted
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                    }
                }

                /* ─── Sobre ─── */
                Section { title: qsTr("Sobre") }

                FieldRow {
                    label: qsTr("Versão")
                    Layout.fillWidth: true
                    Text {
                        text: Qt.application.version
                        color: Theme.fgMuted
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                    }
                }
                FieldRow {
                    label: qsTr("Atualizações")
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("Verificar agora")
                        flat: true
                        onClicked: updater.checkNow()
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceTop : "transparent"
                            border.color: Theme.border
                            radius: Theme.radiusSm
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        }
                        contentItem: Text {
                            text: parent.text
                            color: Theme.accent
                            font.family: Theme.fontSans
                            font.pixelSize: 12
                            leftPadding: 12; rightPadding: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Item { Layout.preferredHeight: 12 }   // bottom spacer
            }
        }
    }

    /* ─── Inline helpers ─── */
    component Section: ColumnLayout {
        property string title
        Layout.fillWidth: true
        Layout.leftMargin: 18
        Layout.rightMargin: 18
        spacing: 8
        Item { Layout.preferredHeight: 4 }
        Text {
            text: title
            color: Theme.fgStrong
            font.family: Theme.fontSans
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.6 }
    }

    component FieldRow: RowLayout {
        property string label
        Layout.fillWidth: true
        Layout.leftMargin: 18
        Layout.rightMargin: 18
        spacing: 12
        Text {
            text: label
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 12
            Layout.preferredWidth: 100
        }
    }
}
