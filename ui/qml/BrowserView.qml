// SPEC-022 — Browser pane. Mirrors the Swift `BrowserPaneView`:
// header strip with back / forward / reload + URL field + close, a
// `WebEngineView` body, and a thin progress bar that fades in/out
// during navigation.
//
// AppState owns the URL per tab via `tabBrowserUrl(tabId)` /
// `setTabBrowserUrl(tabId, url)` Q_INVOKABLE accessors (see the
// integration doc). Drag-and-drop of a link or url-string triggers
// navigation; right-click on the page exposes copy/reload/external.
import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5
import QtWebEngine
import "."

Rectangle {
    id: root
    color: Theme.ink
    focus: true

    readonly property string activeId: appState.activeTabId

    // Re-evaluated whenever AppState fires tabsChanged or active tab flips.
    property int _bump: 0
    readonly property string browserUrl: (_bump, appState.tabBrowserUrl(activeId))

    Connections {
        target: appState
        ignoreUnknownSignals: true
        function onTabsChanged()        { root._bump += 1 }
        function onActiveTabIdChanged() { root._bump += 1 }
    }

    // Coerce a free-form user string to a navigable URL. If it already has
    // a scheme we trust it; otherwise we try "https://" and fall back to a
    // DuckDuckGo search when the input has spaces / no dot.
    function normalize(s) {
        if (!s || s.length === 0) return ""
        const t = s.trim()
        if (/^[a-zA-Z][a-zA-Z0-9+\-.]*:\/\//.test(t)) return t
        if (t.indexOf(" ") < 0 && t.indexOf(".") >= 0) return "https://" + t
        return "https://duckduckgo.com/?q=" + encodeURIComponent(t)
    }

    // Mirror the AppState url into a local QML property so the WebEngineView
    // doesn't churn when the user is mid-typing in the URL bar.
    property url currentUrl: browserUrl.length > 0 ? browserUrl : "about:blank"
    onBrowserUrlChanged: {
        const target = browserUrl.length > 0 ? browserUrl : "about:blank"
        if (currentUrl.toString() !== target) currentUrl = target
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ────────────────────────────────────────────────────────────────
        // Header strip: nav buttons + URL field + close.
        // ────────────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.surfaceLow

            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1; color: Theme.borderSoft
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 6
                spacing: 6

                ToolButton {
                    text: "‹"
                    enabled: web.canGoBack
                    opacity: enabled ? 1.0 : 0.35
                    ToolTip.text: qsTr("Voltar")
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    onClicked: web.goBack()
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fg
                        font.pixelSize: 18
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                    }
                }
                ToolButton {
                    text: "›"
                    enabled: web.canGoForward
                    opacity: enabled ? 1.0 : 0.35
                    ToolTip.text: qsTr("Avançar")
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    onClicked: web.goForward()
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fg
                        font.pixelSize: 18
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                    }
                }
                ToolButton {
                    text: web.loading ? "✕" : "⟳"
                    ToolTip.text: web.loading ? qsTr("Parar") : qsTr("Recarregar")
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    onClicked: {
                        if (web.loading) web.stop()
                        else             web.reload()
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fg
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                    }
                }

                // URL bar. While focused we don't push WebEngineView's
                // url-change events into the field (prevents thrashing
                // mid-edit).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    color: Theme.surface
                    radius: Theme.radiusSm
                    border.color: urlField.activeFocus
                                  ? Theme.accentSoft
                                  : Theme.borderSoft

                    TextField {
                        id: urlField
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        font.family: Theme.fontMono
                        font.pixelSize: 12
                        color: Theme.fg
                        selectByMouse: true
                        placeholderText: qsTr("URL ou busca…")
                        placeholderTextColor: Theme.fgFaint
                        background: Item {}

                        text: root.currentUrl.toString() === "about:blank"
                              ? ""
                              : root.currentUrl.toString()

                        property bool editing: activeFocus

                        Keys.onReturnPressed: submit()
                        Keys.onEnterPressed:  submit()

                        function submit() {
                            const u = root.normalize(text)
                            if (u.length === 0) return
                            root.currentUrl = u
                            appState.setTabBrowserUrl(root.activeId, u)
                            web.forceActiveFocus()
                        }

                        // DnD on the URL field itself.
                        DropArea {
                            anchors.fill: parent
                            onDropped: function(e) {
                                if (e.hasUrls && e.urls.length > 0) {
                                    const u = e.urls[0].toString()
                                    urlField.text = u
                                    urlField.submit()
                                    e.accepted = true
                                } else if (e.hasText) {
                                    urlField.text = e.text
                                    urlField.submit()
                                    e.accepted = true
                                }
                            }
                        }
                    }
                }

                ToolButton {
                    text: "✕"
                    ToolTip.text: qsTr("Fechar tab")
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    onClicked: appState.closeTab(root.activeId)
                    contentItem: Text {
                        text: parent.text
                        color: Theme.fgMuted
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceTop : "transparent"
                        radius: Theme.radiusSm
                    }
                }
            }
        }

        // ────────────────────────────────────────────────────────────────
        // Body: WebEngineView + thin progress bar overlaid at the top.
        // ────────────────────────────────────────────────────────────────
        Item {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            WebEngineView {
                id: web
                anchors.fill: parent
                url: root.currentUrl

                onUrlChanged: {
                    const s = url.toString()
                    if (s.length === 0 || s === "about:blank") return
                    root.currentUrl = url
                    appState.setTabBrowserUrl(root.activeId, s)
                }

                // Right-click context menu — minimal set, matches the
                // Swift sibling's flow.
                onContextMenuRequested: function(req) {
                    req.accepted = true
                    ctxMenu.popup()
                }
            }

            // Progress bar — fades in while loading, out otherwise.
            Rectangle {
                id: progressTrack
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 2
                color: "transparent"
                opacity: web.loading ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: Theme.motionStd } }

                Rectangle {
                    id: progressFill
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width * Math.max(0, Math.min(1, web.loadProgress / 100))
                    color: Theme.accent
                    Behavior on width { NumberAnimation { duration: Theme.motionFast } }
                }
            }

            // Drag-and-drop of a URL onto the body navigates the tab.
            DropArea {
                anchors.fill: parent
                onEntered: function(e) {
                    if (e.hasUrls || e.hasText) e.accepted = true
                }
                onDropped: function(e) {
                    if (e.hasUrls && e.urls.length > 0) {
                        const u = e.urls[0].toString()
                        root.currentUrl = u
                        appState.setTabBrowserUrl(root.activeId, u)
                        e.accepted = true
                    } else if (e.hasText) {
                        const u = root.normalize(e.text)
                        if (u.length > 0) {
                            root.currentUrl = u
                            appState.setTabBrowserUrl(root.activeId, u)
                            e.accepted = true
                        }
                    }
                }
            }

            Menu {
                id: ctxMenu
                MenuItem {
                    text: qsTr("Copiar URL")
                    onTriggered: {
                        const s = root.currentUrl.toString()
                        if (s.length > 0) {
                            urlField.text = s
                            urlField.selectAll()
                            urlField.copy()
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Recarregar")
                    onTriggered: web.reload()
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Abrir no navegador externo")
                    onTriggered: {
                        const s = root.currentUrl.toString()
                        if (s.length > 0) Qt.openUrlExternally(s)
                    }
                }
            }
        }
    }
}
