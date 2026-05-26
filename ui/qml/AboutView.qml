// SPEC-141 — About window. Mirrors the Swift AboutView: app glyph, name,
// version (read from Qt.application.version, kept in sync with main.cpp),
// short tagline, two-row credits, links footer.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

MovablePopup {
    id: root
    width: 460
    height: 420
    minWidth: 380
    minHeight: 320
    title: qsTr("Sobre o Dante CLI")
    icon: "ℹ"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        // ─── Hero glyph ───
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 88; height: 88
            radius: 18
            color: Theme.accent
            Text {
                anchors.centerIn: parent
                text: "D"
                color: "white"
                font.pixelSize: 52
                font.family: Theme.fontSans
                font.weight: Font.Black
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Dante CLI"
            color: Theme.fgStrong
            font.family: Theme.fontSans
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Terminal premium · Mac & Windows")
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 12
        }
        Rectangle {
            Layout.preferredHeight: 24
            Layout.preferredWidth: versionText.implicitWidth + 16
            Layout.alignment: Qt.AlignHCenter
            radius: 10
            color: Theme.surfaceLow
            border.color: Theme.borderSoft
            Text {
                id: versionText
                anchors.centerIn: parent
                text: qsTr("Versão %1").arg(Qt.application.version)
                color: Theme.fg
                font.family: Theme.fontMono
                font.pixelSize: 11
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 6
            color: Theme.borderSoft
            opacity: 0.5
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Port C++20 / Qt 6 / QML do app oficial em Swift. ConPTY no Windows + forkpty no Mac, AI streaming via Groq, paleta de comandos, grid multi-painel, ~47 esquemas de cor.")
            color: Theme.fgMuted
            font.family: Theme.fontSans
            font.pixelSize: 12
        }

        Item { Layout.fillHeight: true }   // spacer

        // ─── Footer links ───
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            LinkBtn { label: qsTr("Site");      url: "https://dantetesta.com.br/dante-cli" }
            LinkBtn { label: qsTr("Repo");      url: "https://github.com/dantetesta/Dante-CLI-Qt" }
            LinkBtn { label: qsTr("Releases");  url: "https://github.com/dantetesta/Dante-CLI-Qt/releases" }
            Item { Layout.fillWidth: true }
            Text {
                text: "© Dante Testa"
                color: Theme.fgFaint
                font.family: Theme.fontSans
                font.pixelSize: 11
            }
        }
    }

    component LinkBtn: Rectangle {
        property string label
        property string url
        radius: 6
        color: linkMa.containsMouse ? Theme.surfaceTop : "transparent"
        border.color: linkMa.containsMouse ? Theme.borderSoft : "transparent"
        implicitWidth: lbl.implicitWidth + 14
        implicitHeight: 24
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
        Text {
            id: lbl
            anchors.centerIn: parent
            text: label
            color: Theme.accent
            font.family: Theme.fontSans
            font.pixelSize: 11
            font.weight: Font.Medium
        }
        MouseArea {
            id: linkMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Qt.openUrlExternally(url)
        }
    }
}
