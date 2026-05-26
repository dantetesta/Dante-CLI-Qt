// Prompt for a video URL (YouTube/Vimeo) or local file path. Matches the
// Swift sibling's "Abrir vídeo no slot" dialog.
//
// On Aplicar/Abrir we just open via QDesktopServices for now — embedding a
// real player needs QtMultimedia QML (for files) or QtWebEngine (for
// YouTube), both of which add ~80 MB to the installer; deferred until the
// user explicitly wants them.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Dialogs   // Qt 6 — replaces Qt.labs.platform
import QtQuick.Layouts 6.5
import "."

MovablePopup {
    id: root
    width: 480
    height: 280
    minWidth: 360
    minHeight: 220
    title: qsTr("Abrir vídeo no slot")
    icon: "🎬"

    /// Emitted when the user confirms; payload is either a URL string or a
    /// file:// path. GridWorkspace listens and binds the cell to this video.
    signal videoChosen(string source)

    onOpened: { urlField.text = ""; urlField.forceActiveFocus() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        // Title handled by MovablePopup header — keeping only the subtitle hint.
        Text {
            visible: false
            text: qsTr("Abrir vídeo no slot")
            color: Theme.fgStrong
            font.family: Theme.fontSans
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
        Text {
            text: qsTr("Aceitamos apenas YouTube, Vimeo ou arquivos de vídeo locais (mp4, mov, m4v, avi, mkv, webm…).")
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        TextField {
            id: urlField
            Layout.fillWidth: true
            placeholderText: "https://www.youtube.com/watch?v=…"
            color: Theme.fg
            selectByMouse: true
            background: Rectangle {
                color: Theme.surfaceLow
                border.color: Theme.accent
                border.width: 1.5
                radius: Theme.radiusSm
            }
            Keys.onReturnPressed: openBtn.clicked()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "📂  " + qsTr("Escolher arquivo local")
                flat: true
                onClicked: fileDlg.open()
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceTop : "transparent"
                    radius: Theme.radiusSm
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.fgMuted
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    leftPadding: 8; rightPadding: 8
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item { Layout.fillWidth: true }

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
                    leftPadding: 12; rightPadding: 12
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Button {
                id: openBtn
                text: qsTr("Abrir")
                enabled: urlField.text.trim().length > 0
                onClicked: {
                    const v = urlField.text.trim()
                    root.videoChosen(v)
                    root.close()
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

    FileDialog {
        id: fileDlg
        title: qsTr("Selecionar vídeo")
        nameFilters: [qsTr("Vídeos (*.mp4 *.mov *.m4v *.avi *.mkv *.webm)"), qsTr("Todos (*)")]
        onAccepted: {
            // Qt 6 QtQuick.Dialogs.FileDialog exposes `selectedFile` (QUrl).
            urlField.text = selectedFile.toString()
        }
    }
}
