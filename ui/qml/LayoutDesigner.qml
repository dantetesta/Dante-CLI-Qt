// Visual split-layout designer — port of the Swift SplitWorkspacePicker.
// Shows a cols×rows preview grid where the user can:
//   • bump column/row counts up to 6
//   • merge neighbor cells by clicking the arrow controls between them
//   • undo a merge by clicking the × in the center of the merged area
//   • assign tabs to slots via the checkbox list at the bottom
// "Aplicar" maps the chosen layout to the existing 2-pane SplitContainer when
// the result is exactly 2 visible panels (Nx1 or 1xN); larger grids are
// previewed but call appState.splitActive("") + show a coming-soon toast
// because the underlying SplitContainer is hardcoded to max 2 panes for now.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Popup {
    id: root

    /* ─── Edit state ──────────────────────────────────────────────────── */
    property int  cols: 2
    property int  rows: 1
    /// Map: cellIndex → {cols, rows}  (default {1,1} when missing).
    /// Mirrors the Swift `spans: [Int: CellSpan]`.
    property var  spans: ({})
    /// Map: cellIndex → tabId.  When the cell is unassigned this maps to "".
    property var  assignments: ({})

    /// Set when the user loaded an existing template — Salvar will update it
    /// in place instead of creating a new one. Cleared when cols/rows/spans
    /// change to something genuinely new.
    property string editingTemplateId: ""
    property string editingTemplateName: ""

    /* ─── Look ─── */
    modal: true
    focus: true
    width: 720
    padding: 18
    background: Rectangle {
        color: Theme.surfaceHigh
        border.color: Theme.borderStrong
        border.width: 1
        radius: Theme.radiusLg
    }
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionStd; easing.type: Easing.OutCubic } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    /* ─── Geometry helpers ────────────────────────────────────────────── */

    function spanAt(idx) {
        const s = root.spans[idx]
        if (s && s.cols && s.rows) return s
        return { cols: 1, rows: 1 }
    }

    /// Indices of cells that are hidden because they were absorbed by a
    /// neighbor's span (mirrors SplitWorkspace.coveredSlots).
    function coveredSet() {
        const covered = {}
        for (let i = 0; i < root.cols * root.rows; ++i) {
            const s = spanAt(i)
            if (s.cols === 1 && s.rows === 1) continue
            const r = Math.floor(i / root.cols)
            const c = i % root.cols
            for (let rr = r; rr < Math.min(root.rows, r + s.rows); ++rr) {
                for (let cc = c; cc < Math.min(root.cols, c + s.cols); ++cc) {
                    const idx = rr * root.cols + cc
                    if (idx !== i) covered[idx] = true
                }
            }
        }
        return covered
    }

    function visibleCount() {
        const cov = coveredSet()
        let n = root.cols * root.rows
        for (const k in cov) n -= 1
        return n
    }

    function clearSpans() {
        root.spans = ({})
    }

    function tryMergeRight(idx) {
        const s = spanAt(idx)
        const r = Math.floor(idx / root.cols)
        const c = idx % root.cols
        if (c + s.cols >= root.cols) return    // already at right edge
        // Merging right means extending this cell into the neighbor on the
        // right (which itself must currently be a 1×1 unmerged cell at
        // matching row position).
        const ns = { cols: s.cols + 1, rows: s.rows }
        root.spans = Object.assign({}, root.spans, { [idx]: ns })
    }
    function tryMergeDown(idx) {
        const s = spanAt(idx)
        const r = Math.floor(idx / root.cols)
        const c = idx % root.cols
        if (r + s.rows >= root.rows) return
        const ns = { cols: s.cols, rows: s.rows + 1 }
        root.spans = Object.assign({}, root.spans, { [idx]: ns })
    }
    function unmerge(idx) {
        if (!root.spans[idx]) return
        const next = Object.assign({}, root.spans)
        delete next[idx]
        root.spans = next
    }

    /* ─── Hydrate when opening ───────────────────────────────────────── */
    onOpened: {
        // Seed from current tab's split state.
        const mode = appState.tabSplitMode(appState.activeTabId)
        if (mode === "vertical")        { root.cols = 2; root.rows = 1 }
        else if (mode === "horizontal") { root.cols = 1; root.rows = 2 }
        else                            { root.cols = 2; root.rows = 1 }
        root.spans = ({})
        root.assignments = ({})
        root.editingTemplateId = ""
        root.editingTemplateName = ""
    }

    /* ─── Apply / Cancel / Exit ──────────────────────────────────────── */

    signal applied()
    signal needsToastNotImplemented(string reason)

    function apply() {
        // Map to existing 2-pane logic when possible.
        const vis = visibleCount()
        if (vis === 1) {
            // No split — collapse back to single pane.
            const sb = appState.tabSecondSessionId(appState.activeTabId)
            if (sb) terminal.kill(sb)
            appState.splitActive("")
        } else if (root.cols === 2 && root.rows === 1 && Object.keys(root.spans).length === 0) {
            appState.splitActive("vertical")
        } else if (root.cols === 1 && root.rows === 2 && Object.keys(root.spans).length === 0) {
            appState.splitActive("horizontal")
        } else {
            root.needsToastNotImplemented(
                qsTr("Grades %1×%2 (e mesclas) ainda usam o motor de 2 painéis — em breve.")
                    .arg(root.cols).arg(root.rows))
        }
        root.applied()
        root.close()
    }

    function exitWorkspace() {
        // Mirror Swift "Sair" — collapse back to single pane (kills b-PTY).
        const sb = appState.tabSecondSessionId(appState.activeTabId)
        if (sb) terminal.kill(sb)
        appState.splitActive("")
        root.close()
    }

    /* ─── Layout ──────────────────────────────────────────────────────── */

    contentItem: ColumnLayout {
        spacing: 14

        /* Header */
        RowLayout {
            spacing: 10
            Text {
                text: qsTr("Visão Dividida")
                color: Theme.fgStrong
                font.family: Theme.fontSans
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Rectangle {
                visible: appState.tabSplitMode(appState.activeTabId) !== ""
                radius: 8
                color: Theme.accentDim
                border.color: Theme.accentSoft
                Layout.preferredHeight: 18
                implicitWidth: badgeText.implicitWidth + 14
                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: qsTr("split ativo")
                    color: Theme.accent
                    font.pixelSize: 10
                    font.family: Theme.fontSans
                    font.weight: Font.DemiBold
                }
            }
            Item { Layout.fillWidth: true }
        }

        /* Templates */
        Text {
            text: qsTr("Templates salvos")
            color: Theme.fg
            font.family: Theme.fontSans
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            radius: Theme.radiusMd
            color: Theme.surface
            border.color: Theme.borderSoft

            // Empty-state hint.
            Text {
                anchors.centerIn: parent
                text: qsTr("Salve um layout pra reutilizar (botão Salvar abaixo)")
                color: Theme.fgFaint
                font.pixelSize: 11
                visible: !templates || templates.list.length === 0
            }

            // Horizontal scroll of cards.
            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                visible: templates && templates.list.length > 0

                Row {
                    spacing: 8
                    Repeater {
                        model: templates ? templates.list : []
                        delegate: Rectangle {
                            id: card
                            width: 110
                            height: 78
                            radius: 8
                            color: cardMa.containsMouse ? Theme.surfaceTop : Theme.surfaceLow
                            border.color: Theme.borderSoft
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 2
                                Text {
                                    text: (modelData.emoji && modelData.emoji.length > 0 ? modelData.emoji + " " : "") + (modelData.name || "")
                                    color: Theme.fgStrong
                                    font.family: Theme.fontSans
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: "%1×%2 · %3 painéis"
                                        .arg(modelData.cols).arg(modelData.rows)
                                        .arg(templates.visiblePanelCount(modelData.cols, modelData.rows, modelData.spans))
                                    color: Theme.fgFaint
                                    font.family: Theme.fontMono
                                    font.pixelSize: 9
                                }
                                Item { Layout.fillHeight: true }
                                // Mini-thumbnail strip (just shows cols×rows count visually).
                                RowLayout {
                                    spacing: 2
                                    Layout.fillWidth: true
                                    Repeater {
                                        model: Math.min(6, modelData.cols * modelData.rows)
                                        delegate: Rectangle {
                                            Layout.fillWidth: true
                                            height: 8
                                            radius: 2
                                            color: Theme.accentSoft
                                        }
                                    }
                                }
                            }

                            // Tiny × delete button (visible on hover).
                            Rectangle {
                                width: 16; height: 16; radius: 8
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: -4
                                color: delMa.containsMouse ? Theme.danger : Theme.surfaceHigh
                                visible: cardMa.containsMouse || delMa.containsMouse
                                border.color: Theme.border
                                Text { anchors.centerIn: parent; text: "×"; color: "white"; font.pixelSize: 10; font.bold: true }
                                MouseArea {
                                    id: delMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: templates.remove(modelData.id)
                                }
                            }

                            MouseArea {
                                id: cardMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // Load this template into the editor.
                                    root.cols = modelData.cols
                                    root.rows = modelData.rows
                                    // Clone the spans map (defensive — modelData.spans is a JS view of QVariantMap).
                                    const cloned = {}
                                    for (const k in modelData.spans) cloned[k] = modelData.spans[k]
                                    root.spans = cloned
                                    root.editingTemplateId = modelData.id
                                    root.editingTemplateName = modelData.name
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.5 }

        /* Layout header */
        RowLayout {
            Text {
                text: qsTr("Layout")
                color: Theme.fgStrong
                font.family: Theme.fontSans
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("%1 painéis").arg(visibleCount())
                color: Theme.fgMuted
                font.family: Theme.fontSans
                font.pixelSize: 11
            }
        }

        /* Editor + steppers */
        RowLayout {
            spacing: 18

            /* Left: grid editor */
            Rectangle {
                Layout.preferredWidth: 380
                Layout.preferredHeight: 240
                color: "transparent"

                // Cell painting via Repeater. Visible cells span (cols×rows)
                // logical units of a fixed-grid render rect.
                Item {
                    id: gridArea
                    anchors.fill: parent
                    anchors.margins: 6
                    readonly property real cellW: (width  - (root.cols - 1) * 4) / root.cols
                    readonly property real cellH: (height - (root.rows - 1) * 4) / root.rows

                    Repeater {
                        model: root.cols * root.rows
                        delegate: Item {
                            id: cellWrap
                            // covered cells render nothing.
                            visible: !root.coveredSet()[index]
                            property var span: root.spanAt(index)
                            x: (index % root.cols) * (gridArea.cellW + 4)
                            y: Math.floor(index / root.cols) * (gridArea.cellH + 4)
                            width:  span.cols * gridArea.cellW + (span.cols - 1) * 4
                            height: span.rows * gridArea.cellH + (span.rows - 1) * 4

                            Rectangle {
                                anchors.fill: parent
                                radius: 8
                                color: Qt.rgba(0.302, 0.435, 0.882, 0.35) // accent blue tint
                                border.color: Theme.accent
                                border.width: 1
                            }

                            // X — undo merge (center of merged cell)
                            Rectangle {
                                visible: cellWrap.span.cols > 1 || cellWrap.span.rows > 1
                                anchors.centerIn: parent
                                width: 26; height: 26; radius: 13
                                color: Theme.danger
                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.unmerge(index)
                                }
                            }

                            // ↔ — merge right (small button on the right edge)
                            Rectangle {
                                visible: (cellWrap.span.cols === 1 && cellWrap.span.rows === 1)
                                       && (index % root.cols) + cellWrap.span.cols < root.cols
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: -10
                                width: 20; height: 20; radius: 10
                                color: Theme.accent
                                border.color: "white"; border.width: 2
                                Text {
                                    anchors.centerIn: parent
                                    text: "↔"
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.tryMergeRight(index)
                                }
                            }
                            // ↕ — merge down
                            Rectangle {
                                visible: (cellWrap.span.cols === 1 && cellWrap.span.rows === 1)
                                       && Math.floor(index / root.cols) + cellWrap.span.rows < root.rows
                                anchors.bottom: parent.bottom
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottomMargin: -10
                                width: 20; height: 20; radius: 10
                                color: Theme.accent
                                border.color: "white"; border.width: 2
                                Text {
                                    anchors.centerIn: parent
                                    text: "↕"
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.tryMergeDown(index)
                                }
                            }
                        }
                    }
                }
            }

            /* Right: steppers + clear-merges */
            ColumnLayout {
                spacing: 14
                Layout.alignment: Qt.AlignTop

                StepperRow { label: qsTr("Colunas"); value: root.cols;
                             onIncreased: if (root.cols < 6) { root.cols += 1; root.spans = ({}) }
                             onDecreased: if (root.cols > 1) { root.cols -= 1; root.spans = ({}) } }

                StepperRow { label: qsTr("Linhas");  value: root.rows;
                             onIncreased: if (root.rows < 6) { root.rows += 1; root.spans = ({}) }
                             onDecreased: if (root.rows > 1) { root.rows -= 1; root.spans = ({}) } }

                Button {
                    text: "↩  " + qsTr("Limpar mesclas")
                    flat: true
                    enabled: Object.keys(root.spans).length > 0
                    onClicked: root.clearSpans()
                    background: Rectangle {
                        color: parent.hovered && parent.enabled ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? Theme.fgMuted : Theme.fgDim
                        font.family: Theme.fontSans
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        /* Hint */
        Text {
            text: qsTr("Setas (↔/↕) entre células mesclam. × no centro de um painel mesclado desfaz.")
            color: Theme.fgFaint
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.5 }

        /* Tabs section */
        RowLayout {
            Text {
                text: qsTr("Abas pra dividir")
                color: Theme.fgStrong
                font.family: Theme.fontSans
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            Rectangle {
                radius: 7
                color: Theme.surfaceTop
                Layout.preferredHeight: 16
                implicitWidth: countText.implicitWidth + 10
                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: qsTr("%1/%2").arg(tabs ? tabs.rowCount() : 0).arg(tabs ? tabs.rowCount() : 0)
                    color: Theme.fgMuted
                    font.pixelSize: 9
                    font.family: Theme.fontMono
                }
            }
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("Todas")
                color: Theme.accent
                font.family: Theme.fontSans
                font.pixelSize: 11
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { /* TODO */ } }
            }
            Text { text: "·"; color: Theme.fgFaint }
            Text {
                text: qsTr("Nenhuma")
                color: Theme.fgMuted
                font.family: Theme.fontSans
                font.pixelSize: 11
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { /* TODO */ } }
            }
        }

        // Simple flat list of tabs as checkboxes (read-only assignment for now).
        Repeater {
            model: tabs
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CheckBox {
                    checked: false
                    onCheckedChanged: { /* TODO: store per-slot assignment */ }
                }
                Text {
                    text: model.emoji && model.emoji.length > 0 ? model.emoji : "🌐"
                    font.pixelSize: 13
                }
                Text {
                    text: model.title || "Terminal"
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.borderSoft; opacity: 0.5 }

        /* Footer */
        RowLayout {
            spacing: 10

            Text {
                text: qsTr("%1 painéis configurados").arg(visibleCount())
                color: Theme.fgFaint
                font.family: Theme.fontSans
                font.pixelSize: 11
            }

            Item { Layout.fillWidth: true }

            // Salvar — opens a small name prompt (or updates the loaded
            // template in place).
            Button {
                text: "🔖  " + (root.editingTemplateId !== "" ? qsTr("Salvar") : qsTr("Salvar como…"))
                flat: true
                enabled: true
                onClicked: saveDialog.open()
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : "transparent"
                    radius: Theme.radiusSm
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: Text { text: parent.text; color: Theme.accent; font.pixelSize: 12; leftPadding: 8; rightPadding: 8; verticalAlignment: Text.AlignVCenter }
            }
            // Sair = collapse the workspace back to single pane.
            Button {
                text: "↪  " + qsTr("Sair")
                flat: true
                enabled: appState.tabSplitMode(appState.activeTabId) !== ""
                onClicked: root.exitWorkspace()
                background: null
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? Theme.danger : Theme.fgDim
                    font.pixelSize: 12
                }
            }
            // Cancelar — close popup with no change.
            Button {
                text: qsTr("Cancelar")
                onClicked: root.close()
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : Theme.surface
                    border.color: Theme.border
                    radius: Theme.radiusSm
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.fg
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12; rightPadding: 12
                }
            }
            // Aplicar — primary
            Button {
                text: qsTr("Aplicar")
                onClicked: root.apply()
                background: Rectangle {
                    color: parent.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                    radius: Theme.radiusSm
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 14; rightPadding: 14
                }
            }
        }
    }

    /* ─── Name prompt (Salvar) ─── */
    Popup {
        id: saveDialog
        modal: true
        focus: true
        anchors.centerIn: parent
        width: 360
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.borderStrong
            border.width: 1
            radius: Theme.radiusMd
        }

        onOpened: {
            nameField.text = root.editingTemplateName.length > 0
                ? root.editingTemplateName
                : qsTr("%1×%2").arg(root.cols).arg(root.rows)
            nameField.forceActiveFocus()
            nameField.selectAll()
        }

        contentItem: ColumnLayout {
            spacing: 14
            Text {
                text: root.editingTemplateId !== ""
                    ? qsTr("Atualizar template")
                    : qsTr("Salvar layout como template")
                color: Theme.fgStrong
                font.family: Theme.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Text {
                    text: qsTr("Nome")
                    color: Theme.fgMuted
                    font.pixelSize: 11
                }
                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    color: Theme.fg
                    placeholderText: qsTr("Ex.: Dev 3 painéis")
                    selectByMouse: true
                    Keys.onReturnPressed: saveBtn.clicked()
                    background: Rectangle {
                        color: Theme.surfaceHigh
                        border.color: Theme.border
                        border.width: 1
                        radius: Theme.radiusSm
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Cancelar")
                    onClicked: saveDialog.close()
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : Theme.surfaceHigh
                        border.color: Theme.border
                        radius: Theme.radiusSm
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: Text { text: parent.text; color: Theme.fg; font.pixelSize: 12; leftPadding: 12; rightPadding: 12; verticalAlignment: Text.AlignVCenter }
                }
                Button {
                    id: saveBtn
                    text: qsTr("Salvar")
                    enabled: nameField.text.trim().length > 0
                    onClicked: {
                        const newId = templates.save(
                            root.editingTemplateId,
                            nameField.text,
                            "",
                            root.cols,
                            root.rows,
                            root.spans
                        )
                        if (newId && newId.length > 0) {
                            root.editingTemplateId = newId
                            root.editingTemplateName = nameField.text
                        }
                        saveDialog.close()
                    }
                    background: Rectangle {
                        color: parent.enabled
                            ? (parent.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
                            : Theme.surfaceTop
                        radius: Theme.radiusSm
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "white" : Theme.fgDim
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        leftPadding: 14; rightPadding: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // ─── Local components ───
    component StepperRow: RowLayout {
        property string label
        property int value
        signal increased()
        signal decreased()
        spacing: 8

        Text {
            text: label
            color: Theme.fgStrong
            font.family: Theme.fontSans
            font.pixelSize: 12
            Layout.preferredWidth: 70
        }

        Rectangle {
            Layout.preferredWidth: 130
            Layout.preferredHeight: 34
            radius: Theme.radiusMd
            color: Theme.surface
            border.color: Theme.border
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                spacing: 0

                StepBtn { glyph: "−"; onClicked: decreased() }
                Text {
                    Layout.fillWidth: true
                    text: value
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                StepBtn { glyph: "+"; onClicked: increased() }
            }
        }
    }
    component StepBtn: Rectangle {
        property string glyph
        signal clicked()
        Layout.preferredWidth: 26
        Layout.preferredHeight: 26
        radius: 4
        color: btnMa.containsMouse ? Theme.surfaceTop : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
        Text {
            anchors.centerIn: parent
            text: glyph
            color: Theme.fg
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
        MouseArea {
            id: btnMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
