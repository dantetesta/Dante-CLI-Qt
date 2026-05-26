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
import "."

Item {
    id: root

    /// Grid shape — populated by Terminal.qml from AppState when a grid is
    /// active for the current tab. Default is a clean 1×1 (single slot).
    property int  cols: 2
    property int  rows: 1
    property var  spans: ({})       // cellIndex → {cols,rows}

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

            EmptySlot {
                anchors.fill: parent
                onRequested: (kind) => root.handleRequest(kind, index)
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
