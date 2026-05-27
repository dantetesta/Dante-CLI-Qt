// SPEC-080 — Dynamic form built from `autoFill.pendingPlaceholders`.
//
// The dialog auto-opens whenever the controller has pending placeholders,
// and auto-closes whenever the list goes back to empty (cancel/submit).
// Each row is rendered with a field appropriate for its `kind`:
//
//   text       — TextField
//   password   — TextField + echoMode=Password + eye toggle
//   path       — TextField + "Procurar" → FileDialog
//   dir        — TextField + "Procurar" → FolderDialog
//   choice     — ComboBox bound to choices[]
//   number     — SpinBox (0..1e9)
//
// Validation: required fields with empty values block Run and surface an
// inline red message. The visible error clears as soon as the user types.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import QtQuick.Dialogs 6.5

MovablePopup {
    id: root
    width: 520
    minWidth: 420
    minHeight: 280
    title: qsTr("Preencher comando")
    icon: "✎"

    // Driven by the controller; we open whenever there are placeholders to
    // collect, and close when the list resets. Mirrors how Swift's sheet
    // presentation key on a non-empty bound state.
    visible: typeof autoFill !== "undefined"
             && autoFill
             && autoFill.pendingPlaceholders
             && autoFill.pendingPlaceholders.length > 0

    onAboutToHide: {
        // If the user dismissed via Esc / × button, cancel the controller
        // so the pending state is cleared.
        if (typeof autoFill !== "undefined" && autoFill
                && autoFill.pendingPlaceholders.length > 0) {
            autoFill.cancel()
        }
    }

    // values cache — populated from defaults whenever pendingPlaceholders
    // changes, then mutated as the user edits each field.
    property var values: ({})
    // last validation error (placeholder name). Empty when all good.
    property string missing: ""

    function resetValues() {
        const next = {}
        const list = (typeof autoFill !== "undefined" && autoFill)
                ? autoFill.pendingPlaceholders : []
        for (let i = 0; i < list.length; ++i) {
            const p = list[i]
            next[p.name] = p.defaultValue || ""
        }
        values = next
        missing = ""
    }

    // Wait one binding cycle so the Repeater rebuilds its children with
    // the fresh model before we try to focus.
    Connections {
        target: typeof autoFill !== "undefined" ? autoFill : null
        function onPendingPlaceholdersChanged() { root.resetValues() }
    }
    Component.onCompleted: resetValues()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: qsTr("Esses campos vêm do comando. Preencha e pressione Executar.")
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        // ─── Dynamic form ───
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                Repeater {
                    id: rep
                    model: (typeof autoFill !== "undefined" && autoFill)
                           ? autoFill.pendingPlaceholders : []

                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        property var p: modelData
                        property bool isMissing: root.missing === p.name

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Text {
                                text: p.label
                                color: Theme.fg
                                font.family: Theme.fontSans
                                font.pixelSize: 12
                                font.weight: Font.Medium
                            }
                            Text {
                                visible: p.required
                                text: "*"
                                color: Theme.danger
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                visible: !p.required && p.defaultValue
                                text: qsTr("padrão: %1").arg(p.defaultValue)
                                color: Theme.fgFaint
                                font.family: Theme.fontMono
                                font.pixelSize: 10
                            }
                        }

                        Loader {
                            id: fieldLoader
                            Layout.fillWidth: true
                            sourceComponent: {
                                if (p.kind === "password") return passwordField
                                if (p.kind === "path")     return pathField
                                if (p.kind === "dir")      return dirField
                                if (p.kind === "choice")   return choiceField
                                if (p.kind === "number")   return numberField
                                return textField
                            }
                            onLoaded: {
                                if (item && item.hasOwnProperty("placeholderName")) {
                                    item.placeholderName = p.name
                                }
                                if (item && item.hasOwnProperty("placeholderInfo")) {
                                    item.placeholderInfo = p
                                }
                            }
                        }

                        Text {
                            visible: isMissing
                            text: qsTr("Obrigatório")
                            color: Theme.danger
                            font.family: Theme.fontSans
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }

        // ─── Buttons ───
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancelar")
                flat: true
                onClicked: {
                    if (typeof autoFill !== "undefined" && autoFill) autoFill.cancel()
                }
            }

            Button {
                id: runBtn
                text: qsTr("Executar")
                onClicked: {
                    if (typeof autoFill === "undefined" || !autoFill) return
                    // Inline required-check.
                    const list = autoFill.pendingPlaceholders
                    for (let i = 0; i < list.length; ++i) {
                        const p = list[i]
                        if (!p.required) continue
                        const v = root.values[p.name]
                        if ((!v || v.length === 0) && !p.defaultValue) {
                            root.missing = p.name
                            return
                        }
                    }
                    root.missing = ""
                    autoFill.submit(root.values)
                }
                background: Rectangle {
                    color: runBtn.hovered ? Theme.accent : Theme.accentSoft
                    radius: Theme.radiusSm
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: Text {
                    text: runBtn.text
                    color: Theme.fgStrong
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // ──────── Field components ────────
    // Each component exposes `placeholderName` so it can mutate
    // root.values[placeholderName] without needing parent property access.
    Component {
        id: textField
        TextField {
            property string placeholderName
            property var placeholderInfo
            text: (root.values[placeholderName] !== undefined) ? root.values[placeholderName] : ""
            placeholderText: placeholderInfo ? placeholderInfo.label : ""
            font.family: Theme.fontMono
            font.pixelSize: 12
            color: Theme.fg
            onTextChanged: {
                const v = root.values
                v[placeholderName] = text
                root.values = v
                if (root.missing === placeholderName) root.missing = ""
            }
        }
    }

    Component {
        id: passwordField
        RowLayout {
            property string placeholderName
            property var placeholderInfo
            spacing: 4
            TextField {
                id: pwd
                Layout.fillWidth: true
                text: (root.values[parent.placeholderName] !== undefined)
                      ? root.values[parent.placeholderName] : ""
                echoMode: revealed ? TextInput.Normal : TextInput.Password
                property bool revealed: false
                font.family: Theme.fontMono
                font.pixelSize: 12
                color: Theme.fg
                onTextChanged: {
                    const v = root.values
                    v[parent.placeholderName] = text
                    root.values = v
                    if (root.missing === parent.placeholderName) root.missing = ""
                }
            }
            Button {
                text: pwd.revealed ? "🙈" : "👁"
                flat: true
                implicitWidth: 32
                onClicked: pwd.revealed = !pwd.revealed
            }
        }
    }

    Component {
        id: pathField
        RowLayout {
            property string placeholderName
            property var placeholderInfo
            spacing: 4
            TextField {
                id: pathInput
                Layout.fillWidth: true
                text: (root.values[parent.placeholderName] !== undefined)
                      ? root.values[parent.placeholderName] : ""
                placeholderText: qsTr("Caminho do arquivo")
                font.family: Theme.fontMono
                font.pixelSize: 12
                color: Theme.fg
                onTextChanged: {
                    const v = root.values
                    v[parent.placeholderName] = text
                    root.values = v
                    if (root.missing === parent.placeholderName) root.missing = ""
                }
            }
            Button {
                text: qsTr("Procurar")
                onClicked: fileDlg.open()
            }
            FileDialog {
                id: fileDlg
                onAccepted: pathInput.text = selectedFile.toString().replace(/^file:\/\//, "")
            }
        }
    }

    Component {
        id: dirField
        RowLayout {
            property string placeholderName
            property var placeholderInfo
            spacing: 4
            TextField {
                id: dirInput
                Layout.fillWidth: true
                text: (root.values[parent.placeholderName] !== undefined)
                      ? root.values[parent.placeholderName] : ""
                placeholderText: qsTr("Diretório")
                font.family: Theme.fontMono
                font.pixelSize: 12
                color: Theme.fg
                onTextChanged: {
                    const v = root.values
                    v[parent.placeholderName] = text
                    root.values = v
                    if (root.missing === parent.placeholderName) root.missing = ""
                }
            }
            Button {
                text: qsTr("Procurar")
                onClicked: folderDlg.open()
            }
            FolderDialog {
                id: folderDlg
                onAccepted: dirInput.text = selectedFolder.toString().replace(/^file:\/\//, "")
            }
        }
    }

    Component {
        id: choiceField
        ComboBox {
            property string placeholderName
            property var placeholderInfo
            model: placeholderInfo ? placeholderInfo.choices : []
            currentIndex: {
                if (!placeholderInfo) return -1
                const cur = root.values[placeholderName]
                const idx = placeholderInfo.choices.indexOf(cur)
                return idx >= 0 ? idx : 0
            }
            onActivated: {
                const v = root.values
                v[placeholderName] = currentText
                root.values = v
                if (root.missing === placeholderName) root.missing = ""
            }
            Component.onCompleted: {
                if (placeholderInfo && placeholderInfo.choices.length > 0
                        && !root.values[placeholderName]) {
                    const v = root.values
                    v[placeholderName] = placeholderInfo.choices[0]
                    root.values = v
                }
            }
        }
    }

    Component {
        id: numberField
        SpinBox {
            property string placeholderName
            property var placeholderInfo
            from: 0
            to: 1000000000
            editable: true
            value: {
                const v = root.values[placeholderName]
                const n = parseInt(v, 10)
                return isNaN(n) ? 0 : n
            }
            onValueChanged: {
                const v = root.values
                v[placeholderName] = String(value)
                root.values = v
                if (root.missing === placeholderName) root.missing = ""
            }
        }
    }
}
