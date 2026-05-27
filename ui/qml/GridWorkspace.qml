// N×M grid workspace — renders the layout the user designed in
// LayoutDesigner. Each visible cell is an EmptySlot until the user assigns
// content (a tab, video, or browser). Mirrors the Swift sibling's empty
// workspace state shown in the multi-pane screenshot.
//
// Scope for v0.7.0-alpha.10:
//   • Renders cols × rows with merged spans honoring the LayoutDesigner output
//   • Each slot is an EmptySlot whose 4 shortcut buttons fire `requested(kind)`
//   • "terminal" → spawns a new tab via appState.addTab + asks parent to swap
//   • "browser" / "anonymous" → QDesktopServices.openUrl (about:blank / DuckDuckGo)
//   • "video" → opens VideoOpenDialog → QDesktopServices.openUrl
// Real in-pane browser/video rendering needs QtWebEngine + QtMultimedia QML
// modules; will land in a follow-up version.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: root

    /// Grid shape — populated by Terminal.qml from AppState when a grid is
    /// active for the current tab. Default is a clean 1×1 (single slot).
    property int  cols: 2
    property int  rows: 1
    property var  spans: ({})       // cellIndex → {cols,rows}

    /// SPEC-170: id of the tab that hosts this grid. Slots use it to ask
    /// AppState which tab (if any) is mapped to each cell. Empty string =
    /// fall back to appState.activeTabId at query time.
    property string hostTabId: ""
    function effectiveHostId() {
        return hostTabId.length > 0 ? hostTabId : appState.activeTabId
    }

    /// Bumped after a successful drop so the Repeater re-evaluates which
    /// cells should render an embedded tab instead of an EmptySlot.
    property int assignmentsTick: 0

    /// Indices of cells covered by another cell's span.
    function coveredSet() {
        const cov = {}
        for (let i = 0; i < cols * rows; ++i) {
            const s = spans[i]
            if (!s || (s.cols === 1 && s.rows === 1)) continue
            const r = Math.floor(i / cols)
            const c = i % cols
            for (let rr = r; rr < Math.min(rows, r + s.rows); ++rr) {
                for (let cc = c; cc < Math.min(cols, c + s.cols); ++cc) {
                    const idx = rr * cols + cc
                    if (idx !== i) cov[idx] = true
                }
            }
        }
        return cov
    }

    readonly property real gapPx: 6
    readonly property real cellW: (width  - (cols - 1) * gapPx) / Math.max(1, cols)
    readonly property real cellH: (height - (rows - 1) * gapPx) / Math.max(1, rows)

    Repeater {
        model: root.cols * root.rows
        delegate: Item {
            id: cellItem
            visible: !root.coveredSet()[index]
            // Span (1×1 unless explicitly merged).
            property var span: {
                const s = root.spans[index]
                return s && s.cols && s.rows ? s : { cols: 1, rows: 1 }
            }
            x: (index % root.cols) * (root.cellW + root.gapPx)
            y: Math.floor(index / root.cols) * (root.cellH + root.gapPx)
            width:  span.cols * root.cellW + (span.cols - 1) * root.gapPx
            height: span.rows * root.cellH + (span.rows - 1) * root.gapPx

            // SPEC-170: ask AppState whether this cell already has a tab
            // assigned. If yes, render an embedded placeholder representing
            // the attached tab; the live TerminalView swap lands once the
            // C++ side wires the per-cell PTY context (see integration doc).
            // The `assignmentsTick` dependency ensures this property
            // re-evaluates after a drop.
            property string assignedTabId: {
                root.assignmentsTick;   // dep
                const hid = root.effectiveHostId()
                return (hid && hid.length > 0 && appState.tabAtSlot)
                    ? appState.tabAtSlot(hid, index)
                    : ""
            }

            EmptySlot {
                anchors.fill: parent
                visible: cellItem.assignedTabId.length === 0
                hostTabId: root.effectiveHostId()
                cellIndex: index
                onRequested: (kind) => root.handleRequest(kind, index)
                onTabDropped: (sourceTabId, ci) => { root.assignmentsTick += 1 }
            }

            // Lightweight stand-in for the attached tab's content. Renders
            // the tab title + emoji so the user gets immediate confirmation
            // the drop landed. Full TerminalView embedding is described in
            // SPEC-170-INTEGRATION.md.
            Rectangle {
                anchors.fill: parent
                visible: cellItem.assignedTabId.length > 0
                color: Theme.surfaceLow
                border.color: Theme.borderSoft
                border.width: 1
                radius: Theme.radiusSm
                opacity: visible ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: Theme.motionStd; easing.type: Easing.OutCubic } }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "💻"
                        font.pixelSize: 26
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        // We render the tabId by default. Once AppState
                        // gains `tabTitle(QString)` / `tabEmoji(QString)`
                        // Q_INVOKABLEs (see SPEC-170-INTEGRATION.md) this
                        // can be upgraded to a richer card without further
                        // QML changes.
                        text: cellItem.assignedTabId.length > 0
                              ? cellItem.assignedTabId
                              : qsTr("Aba anexada")
                        color: Theme.fgStrong
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("(tab anexada — sessão embarcada chega no próximo build)")
                        color: Theme.fgFaint
                        font.family: Theme.fontSans
                        font.pixelSize: 10
                    }
                }
            }
        }
    }

    /* ─── Shortcut wiring ─── */
    function handleRequest(kind, cellIndex) {
        if (kind === "terminal") {
            // Until we have per-cell sessions, spawn a new tab — it lands in
            // the tab bar and the user can switch to it.
            appState.addTab(qsTr("Terminal"))
        } else if (kind === "browser") {
            Qt.openUrlExternally("https://www.google.com")
        } else if (kind === "anonymous") {
            // No cross-platform "open in incognito" via Qt; we just open the
            // privacy-friendly DuckDuckGo as a stand-in until QtWebEngine
            // gives us a proper in-app private context.
            Qt.openUrlExternally("https://duckduckgo.com")
        } else if (kind === "video") {
            videoDlg.targetCell = cellIndex
            videoDlg.open()
        }
    }

    VideoOpenDialog {
        id: videoDlg
        property int targetCell: -1
        anchors.centerIn: parent
        onVideoChosen: (source) => {
            // Open externally for now (system default video player or YouTube
            // in default browser). Embedded playback lands with QtMultimedia.
            Qt.openUrlExternally(source)
        }
    }
}
