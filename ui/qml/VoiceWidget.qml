// Floating voice-recording HUD. Visible whenever `voice.status` ∈
// {recording, transcribing, error}; fades back out when status returns to
// "idle". Rendered as an overlay child of Main so it sits above the
// terminal but below the AI overlay's modal scrim.
//
// Three visual states:
//   recording    → pulsing red dot + 16 animated bars driven by voice.level
//                  + Parar / Cancelar buttons
//   transcribing → spinner + "Transcrevendo…"
//   error        → red bullet + error string + Fechar
//
// The bars use a small rolling history buffer (`levelHistory`) so the wave
// scrolls naturally — driving every bar off the same instantaneous level
// would just make them all flap in unison.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import "."

Item {
    id: root
    anchors.fill: parent
    z: 950 // above terminal (0), below AI overlay (1000)

    readonly property bool active: voice && voice.status !== "idle"
    visible: opacity > 0.01
    opacity: active ? 1.0 : 0.0
    Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

    // ── Rolling history for the waveform ──────────────────────────────────
    // Push the freshest level onto the front; the bars sample by index.
    property var levelHistory: []
    readonly property int  barCount: 16

    Connections {
        target: voice
        function onLevelChanged() {
            const next = voice.level
            const h = root.levelHistory.slice()
            h.unshift(next)
            if (h.length > root.barCount) h.length = root.barCount
            root.levelHistory = h
        }
        function onStatusChanged() {
            if (voice.status === "idle" || voice.status === "transcribing") {
                // Decay bars back to flat when capture ends.
                root.levelHistory = []
            }
        }
    }

    // ── Click-outside shield (only while recording, so the user can't lose
    //    a take by clicking a stray button behind the HUD). ─────────────────
    MouseArea {
        anchors.fill: parent
        enabled: root.active
        // Swallow but don't dismiss — Cancelar/Parar buttons are the only way out.
        onClicked: {}
    }

    // ── Card ──────────────────────────────────────────────────────────────
    Rectangle {
        id: card
        width: 380
        height: 168
        anchors.centerIn: parent
        radius: Theme.radiusLg
        color: Theme.surface
        border.color: Theme.border
        border.width: 1

        // Subtle drop shadow via stacked rectangle (no QtGraphicalEffects to
        // avoid pulling in the extra module — keeps the dep surface tight).
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.color: Qt.rgba(0,0,0,0.35)
            border.width: 1
            z: -1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            // ── Top row: status indicator + label ──
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                // Pulsing red dot (recording) | spinner (transcribing) | bullet (error)
                Item {
                    Layout.preferredWidth: 14
                    Layout.preferredHeight: 14

                    Rectangle {
                        id: dot
                        anchors.centerIn: parent
                        width: 12; height: 12; radius: 6
                        color: voice && voice.status === "error" ? Theme.danger
                             : voice && voice.status === "recording" ? Theme.danger
                             : Theme.accent
                        visible: !spinner.visible

                        SequentialAnimation on opacity {
                            running: voice && voice.status === "recording"
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.35; duration: 600; easing.type: Easing.InOutSine }
                            NumberAnimation { from: 0.35; to: 1.0; duration: 600; easing.type: Easing.InOutSine }
                        }
                    }

                    BusyIndicator {
                        id: spinner
                        anchors.centerIn: parent
                        width: 16; height: 16
                        running: visible
                        visible: voice && voice.status === "transcribing"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    text: voice
                        ? (voice.status === "recording"    ? "Gravando…"
                         : voice.status === "transcribing" ? "Transcrevendo…"
                         : voice.status === "error"        ? "Erro"
                         : "")
                        : ""
                }
            }

            // ── Waveform ──
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                visible: voice && voice.status === "recording"

                RowLayout {
                    anchors.fill: parent
                    spacing: 4

                    Repeater {
                        model: root.barCount
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            // Bar height: sample the i-th historical level
                            // (fall back to a faint baseline so the row never
                            // collapses entirely while waiting for input).
                            readonly property real sample:
                                index < root.levelHistory.length
                                    ? root.levelHistory[index]
                                    : 0.0
                            Layout.preferredHeight: 6 + sample * 40
                            radius: 2
                            color: Qt.tint(Theme.accent,
                                Qt.rgba(1, 1, 1, 0.0)) // accent at full opacity
                            opacity: 0.45 + sample * 0.55

                            Behavior on Layout.preferredHeight {
                                NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
                            }
                            Behavior on opacity {
                                NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
                            }
                        }
                    }
                }
            }

            // ── Error string (only when status == error) ──
            Text {
                Layout.fillWidth: true
                visible: voice && voice.status === "error"
                wrapMode: Text.WordWrap
                color: Theme.fgMuted
                font.family: Theme.fontSans
                font.pixelSize: 12
                text: voice ? voice.lastError : ""
            }

            Item { Layout.fillHeight: true } // spacer

            // ── Buttons ──
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item { Layout.fillWidth: true }

                // Cancel — recording only.
                Rectangle {
                    visible: voice && voice.status === "recording"
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 90
                    radius: Theme.radiusSm
                    color: cancelMa.containsMouse ? Theme.surfaceHigh : "transparent"
                    border.color: Theme.border
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: "Cancelar"
                        color: Theme.fg
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: cancelMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voice.cancelRecording()
                    }
                }

                // Stop — recording only. Primary action.
                Rectangle {
                    visible: voice && voice.status === "recording"
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 100
                    radius: Theme.radiusSm
                    color: stopMa.containsMouse ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: "Parar"
                        color: Theme.fgStrong
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                    MouseArea {
                        id: stopMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: voice.stopRecording()
                    }
                }

                // Close — error only.
                Rectangle {
                    visible: voice && voice.status === "error"
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 100
                    radius: Theme.radiusSm
                    color: closeMa.containsMouse ? Theme.surfaceHigh : "transparent"
                    border.color: Theme.border
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: "Fechar"
                        color: Theme.fg
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: closeMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        // cancelRecording() also flips status back to idle.
                        onClicked: voice.cancelRecording()
                    }
                }
            }
        }
    }
}
