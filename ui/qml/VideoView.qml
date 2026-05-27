// SPEC-023 — Video pane. Mirrors the Swift `VideoPaneView`:
// MediaPlayer + VideoOutput with header (filename + open + close), an
// auto-fading controls strip (play/pause, scrub, volume + mute, time
// labels, fullscreen) and drag-and-drop to load a file.
//
// AppState owns the path (one per tab) via Q_INVOKABLE accessors that
// the integration doc spells out. We treat the path as an absolute file
// path *or* a "file://" URL — both are normalised before being passed
// to MediaPlayer.source.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import QtMultimedia
import QtQuick.Dialogs
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    readonly property string activeId: appState.activeTabId

    // Re-evaluated whenever AppState fires tabsChanged or active tab flips.
    property int _bump: 0
    readonly property string videoPath: (_bump, appState.tabVideoPath(activeId))

    Connections {
        target: appState
        ignoreUnknownSignals: true
        function onTabsChanged()        { root._bump += 1 }
        function onActiveTabIdChanged() { root._bump += 1 }
    }

    // Normalise a path or url-string to a QUrl for MediaPlayer.source.
    function toUrl(s) {
        if (!s || s.length === 0) return ""
        if (s.indexOf("file://") === 0 || s.indexOf("http://") === 0 || s.indexOf("https://") === 0)
            return s
        return "file://" + s
    }
    function fromUrl(u) {
        const s = (typeof u === "string") ? u : u.toString()
        if (s.indexOf("file://") === 0) return s.substring(7)
        return s
    }

    onVideoPathChanged: {
        const u = toUrl(videoPath)
        if (player.source.toString() !== u) {
            player.source = u
            if (u.length > 0) player.play()
        }
    }
    Component.onCompleted: {
        const u = toUrl(videoPath)
        if (u.length > 0) player.source = u
    }

    /* ────────────────────────────────────────────────────────────────
       Media engine.
       ──────────────────────────────────────────────────────────────── */
    MediaPlayer {
        id: player
        audioOutput: AudioOutput {
            id: audio
            volume: 0.8
            muted: false
        }
        videoOutput: videoOut
        onErrorOccurred: function(err, str) {
            // Surface read errors in the header — controller-level toast
            // wiring lands when AppState grows operationFailed().
            console.warn("[video]", err, str)
        }
    }

    // Format milliseconds → "m:ss" or "h:mm:ss" for longer videos.
    function fmt(ms) {
        if (ms <= 0 || isNaN(ms)) return "0:00"
        const s = Math.floor(ms / 1000)
        const h = Math.floor(s / 3600)
        const m = Math.floor((s % 3600) / 60)
        const sec = s % 60
        const pad = function(n) { return n < 10 ? "0" + n : "" + n }
        return h > 0 ? (h + ":" + pad(m) + ":" + pad(sec))
                     : (m + ":" + pad(sec))
    }

    function basename(p) {
        if (!p) return ""
        const cleaned = fromUrl(p)
        const i = cleaned.lastIndexOf("/")
        return i >= 0 ? cleaned.substring(i + 1) : cleaned
    }

    /* ────────────────────────────────────────────────────────────────
       Layout.
       ──────────────────────────────────────────────────────────────── */
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header: filename + open + close.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: Theme.surfaceLow
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1; color: Theme.borderSoft
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 8

                Text {
                    text: "🎬"
                    font.pixelSize: 13
                }
                Text {
                    Layout.fillWidth: true
                    text: root.videoPath.length > 0
                          ? root.basename(root.videoPath)
                          : qsTr("Nenhum vídeo carregado")
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    elide: Text.ElideMiddle
                }
                ToolButton {
                    text: "📂"
                    ToolTip.text: qsTr("Abrir arquivo")
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    onClicked: fileDlg.open()
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                    }
                }
                ToolButton {
                    text: "✕"
                    ToolTip.text: qsTr("Fechar vídeo")
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    onClicked: {
                        player.stop()
                        player.source = ""
                        appState.setTabVideoPath(root.activeId, "")
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                    }
                }
            }
        }

        // Video surface — clicking toggles play/pause, mouse-move re-shows
        // the overlay.
        Item {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Rectangle { anchors.fill: parent; color: "black" }

            VideoOutput {
                id: videoOut
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
            }

            // Empty-state hint.
            Item {
                anchors.centerIn: parent
                visible: root.videoPath.length === 0
                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        text: "🎬"
                        font.pixelSize: 56
                        color: Theme.fgDim
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Text {
                        text: qsTr("Arraste um vídeo aqui ou clique em 📂")
                        color: Theme.fgMuted
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // Toggle play / pause + revive overlay.
            MouseArea {
                id: stageMa
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                onPositionChanged: revealOverlay()
                onEntered:         revealOverlay()
                onClicked: {
                    if (root.videoPath.length === 0) return
                    if (player.playbackState === MediaPlayer.PlayingState) player.pause()
                    else                                                   player.play()
                }
            }

            // Drag and drop: accept anything with a local file URL.
            DropArea {
                anchors.fill: parent
                onEntered: function(e) {
                    if (e.hasUrls) e.accepted = true
                }
                onDropped: function(e) {
                    if (!e.hasUrls || e.urls.length === 0) return
                    const u = e.urls[0].toString()
                    appState.setTabVideoPath(root.activeId, u)
                }
            }

            // Controls overlay — fades to 0 after 2 s of inactivity. The
            // Timer is restarted by revealOverlay() (called on mouse move).
            Rectangle {
                id: overlay
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 56
                color: Qt.rgba(0, 0, 0, 0.55)
                opacity: 1.0
                Behavior on opacity { NumberAnimation { duration: Theme.motionStd } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    // Play / Pause toggle.
                    ToolButton {
                        text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                        onClicked: {
                            if (player.playbackState === MediaPlayer.PlayingState) player.pause()
                            else                                                   player.play()
                            revealOverlay()
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(1, 1, 1, 0.15) : "transparent"
                            radius: Theme.radiusSm
                        }
                    }

                    // Scrub slider.
                    Slider {
                        id: scrub
                        Layout.fillWidth: true
                        from: 0
                        to: player.duration > 0 ? player.duration : 1
                        value: player.position
                        // While the user drags we suppress the binding so the
                        // thumb doesn't yo-yo back to the last frame.
                        property bool dragging: false
                        onPressedChanged: {
                            dragging = pressed
                            if (!pressed) player.position = value
                            revealOverlay()
                        }
                        onMoved: if (dragging) revealOverlay()
                    }

                    Text {
                        text: root.fmt(scrub.dragging ? scrub.value : player.position)
                              + " / "
                              + root.fmt(player.duration)
                        color: "white"
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                    }

                    // Mute toggle.
                    ToolButton {
                        text: audio.muted ? "🔇" : (audio.volume > 0.5 ? "🔊" : "🔈")
                        onClicked: { audio.muted = !audio.muted; revealOverlay() }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(1, 1, 1, 0.15) : "transparent"
                            radius: Theme.radiusSm
                        }
                    }
                    // Volume slider — width capped so it doesn't crowd the strip.
                    Slider {
                        id: vol
                        Layout.preferredWidth: 90
                        from: 0; to: 1
                        value: audio.volume
                        onMoved: { audio.volume = value; audio.muted = false; revealOverlay() }
                    }

                    // Fullscreen toggle — toggles the QQuickWindow that hosts
                    // the scene. Mirrors Swift's NSWindow.toggleFullScreen.
                    ToolButton {
                        text: "⛶"
                        ToolTip.text: qsTr("Tela cheia")
                        ToolTip.visible: hovered
                        ToolTip.delay: 400
                        onClicked: {
                            const w = root.Window.window
                            if (!w) return
                            if (w.visibility === Window.FullScreen) w.visibility = Window.Windowed
                            else                                    w.visibility = Window.FullScreen
                            revealOverlay()
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(1, 1, 1, 0.15) : "transparent"
                            radius: Theme.radiusSm
                        }
                    }
                }
            }

            // 2 s idle → fade overlay.
            Timer {
                id: hideTimer
                interval: 2000
                repeat: false
                onTriggered: {
                    if (player.playbackState === MediaPlayer.PlayingState
                            && root.videoPath.length > 0) {
                        overlay.opacity = 0.0
                    }
                }
            }
        }
    }

    function revealOverlay() {
        overlay.opacity = 1.0
        hideTimer.restart()
    }

    // Restore overlay whenever playback stops or path changes.
    Connections {
        target: player
        function onPlaybackStateChanged() {
            if (player.playbackState !== MediaPlayer.PlayingState) {
                overlay.opacity = 1.0
                hideTimer.stop()
            } else {
                hideTimer.restart()
            }
        }
    }

    /* ────────────────────────────────────────────────────────────────
       File dialog fallback.
       ──────────────────────────────────────────────────────────────── */
    FileDialog {
        id: fileDlg
        title: qsTr("Selecionar vídeo")
        nameFilters: [qsTr("Vídeos (*.mp4 *.mov *.m4v *.avi *.mkv *.webm)"),
                      qsTr("Todos (*)")]
        onAccepted: appState.setTabVideoPath(root.activeId, selectedFile.toString())
    }
}
