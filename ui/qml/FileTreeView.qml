// Full filesystem tree for the Pastas sidebar mode. Uses Qt 6's QML TreeView
// against the FileTreeController's QFileSystemModel. Right-click on a row
// opens a context menu (abrir, abrir no slot, copiar caminho, etc.). The
// row's MouseArea also exposes a `Drag` source so items can be dropped onto
// GridWorkspace slots.
//
// The header has a path breadcrumb + ⟲ refresh + ↑ go-up + 📁+ new-folder.
// TreeView.rootIndex (used at line ~130) was added in QtQuick.Controls 6.7,
// so this file bumps to 6.7 explicitly. The installer ships Qt 6.7.2.
import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7
import "."
// Qt.labs.platform was imported here but never used; it was breaking on
// Windows in Qt 6.7 where the QML module isn't deployed by default.

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        /* ─── Quick-place strip (Início / Desktop / Downloads / / / drives…) ─── */
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: Theme.surface
            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.borderSoft }

            ListView {
                id: placesList
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                orientation: ListView.Horizontal
                spacing: 4
                clip: true
                model: fileTree.quickPlaces()
                delegate: Rectangle {
                    height: placesList.height - 8
                    width: lbl.implicitWidth + ico.implicitWidth + 18
                    radius: Theme.radiusSm
                    color: (fileTree.rootPath === modelData.path)
                        ? Theme.accentDim
                        : (qpMa.containsMouse ? Theme.surfaceHigh : "transparent")
                    border.color: (fileTree.rootPath === modelData.path) ? Theme.accentSoft : "transparent"
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    Row {
                        anchors.centerIn: parent
                        spacing: 5
                        Text { id: ico; text: modelData.icon; font.pixelSize: 12 }
                        Text {
                            id: lbl
                            text: modelData.label
                            color: (fileTree.rootPath === modelData.path) ? Theme.accent : Theme.fg
                            font.family: Theme.fontSans
                            font.pixelSize: 11
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        id: qpMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: fileTree.rootPath = modelData.path
                    }
                }
            }
        }

        /* ─── Path header / toolbar ─── */
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.surfaceLow
            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.borderSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 6
                spacing: 4

                // ↑ — go to parent directory.
                IconBtn {
                    glyph: "↑"
                    tip: qsTr("Pasta acima")
                    onClicked: {
                        const p = fileTree.rootPath
                        if (p === "/" || p.length === 0) return
                        const cut = p.lastIndexOf("/")
                        fileTree.rootPath = cut <= 0 ? "/" : p.substring(0, cut)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: {
                        const p = fileTree.rootPath
                        if (p === "/") return "/"
                        return p.split("/").pop() || p
                    }
                    color: Theme.fg
                    font.family: Theme.fontSans
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    elide: Text.ElideMiddle
                    ToolTip.text: fileTree.rootPath
                    ToolTip.visible: hoverHandler.hovered
                    ToolTip.delay: 400
                    HoverHandler { id: hoverHandler }
                }
                IconBtn {
                    glyph: fileTree.showHidden ? "👁" : "👁​"
                    tip: fileTree.showHidden ? qsTr("Ocultar arquivos ocultos") : qsTr("Mostrar arquivos ocultos")
                    onClicked: fileTree.showHidden = !fileTree.showHidden
                }
                IconBtn { glyph: "📁+"; tip: qsTr("Nova pasta"); onClicked: newDlg.openFor("folder", fileTree.rootPath) }
                IconBtn { glyph: "📄+"; tip: qsTr("Novo arquivo"); onClicked: newDlg.openFor("file", fileTree.rootPath) }
                IconBtn { glyph: "↻";  tip: qsTr("Recarregar"); onClicked: { const p = fileTree.rootPath; fileTree.rootPath = ""; fileTree.rootPath = p } }
            }
        }

        /* ─── Tree ─── */
        TreeView {
            id: tree
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: fileTree.model
            rootIndex: fileTree.rootIndex()
            // selectionModel: ItemSelectionModel { model: fileTree.model }   // optional — not needed for our minimal interactions

            // Reload-able binding: when fileTree.rootPath changes, refresh
            // both model root and our visible rootIndex.
            Connections {
                target: fileTree
                function onRootChanged() { tree.rootIndex = fileTree.rootIndex() }
            }

            delegate: Item {
                id: row
                implicitWidth: tree.width
                implicitHeight: 26

                // Required TreeView delegate properties — see Qt docs for
                // "Qt Quick TreeView delegate". `display` is the QFileSystem
                // model's default role (= the file name). We pull the path
                // lazily — only when the user does something with the row —
                // because calling the C++ controller per paint caused all
                // visible rows to mis-resolve and show the same name.
                required property TreeView treeView
                required property bool isTreeNode
                required property bool expanded
                required property int  hasChildren
                required property int  depth
                required property int  row
                required property int  column
                required property string display

                readonly property string fileName: display
                // Resolved on demand (right-click, drag, double-click).
                // Computing in a property is fine because the TreeView only
                // re-evaluates it when row/column actually change.
                function modelIndex() { return treeView.index(row, column) }
                function path()       { return fileTree.filePath(modelIndex()) }
                function isDir()      { return fileTree.isDir(modelIndex()) }

                // Hover / selection bg.
                Rectangle {
                    anchors.fill: parent
                    color: rowMa.containsMouse ? Theme.surfaceHigh : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: row.depth * 16 + 4
                    anchors.rightMargin: 4
                    spacing: 4

                    // Disclosure triangle — visible whenever the tree node
                    // *can* have children (QFileSystemModel says yes for all
                    // directories before it has fetched them).
                    Item {
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        visible: row.hasChildren
                        Text {
                            anchors.centerIn: parent
                            text: row.expanded ? "▾" : "▸"
                            color: Theme.fgMuted
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -4
                            cursorShape: Qt.PointingHandCursor
                            onClicked: row.treeView.toggleExpanded(row.row)
                        }
                    }
                    Item { Layout.preferredWidth: 14; visible: !row.hasChildren }

                    // Icon (folder / file glyph).
                    Text {
                        text: row.hasChildren ? "📁" : iconForExtension(row.fileName)
                        font.pixelSize: 13
                    }
                    Text {
                        Layout.fillWidth: true
                        text: row.fileName
                        color: Theme.fg
                        font.family: Theme.fontSans
                        font.pixelSize: 12
                        elide: Text.ElideMiddle
                    }
                }

                /* ─── Click + context + drag ─── */
                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    drag.target: dragGhost
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            menu.targetPath = row.path()
                            menu.targetIsDir = row.isDir()
                            menu.popup()
                        }
                    }
                    onDoubleClicked: {
                        if (row.isDir()) {
                            // Drilling: make this folder the new root (the Swift
                            // sibling does the same — double-click navigates in).
                            fileTree.rootPath = row.path()
                        } else {
                            fileTree.openExternally(row.path())
                        }
                    }
                    onPressed: (mouse) => {
                        if (mouse.button === Qt.LeftButton) {
                            dragGhost.x = mouse.x
                            dragGhost.y = mouse.y
                        }
                    }
                    onReleased: dragGhost.Drag.drop()
                }

                // Drag ghost — small floating chip the user drags to a slot.
                Rectangle {
                    id: dragGhost
                    width: dragLabel.implicitWidth + 18
                    height: 24
                    radius: 6
                    color: Theme.accent
                    opacity: Drag.active ? 0.92 : 0
                    visible: opacity > 0
                    Drag.active: rowMa.drag.active
                    Drag.dragType: Drag.Automatic
                    Drag.supportedActions: Qt.CopyAction
                    Drag.mimeData: { "text/uri-list": "file://" + row.path(),
                                     "text/plain":    row.path() }
                    Text {
                        id: dragLabel
                        anchors.centerIn: parent
                        text: (row.isDir() ? "📁 " : "📄 ") + row.fileName
                        color: "white"
                        font.family: Theme.fontSans
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }
                }
            }
        }
    }

    /* ─── Context menu (right-click) ─── */
    Menu {
        id: menu
        property string targetPath: ""
        property bool   targetIsDir: false

        MenuItem {
            text: menu.targetIsDir ? qsTr("Abrir no terminal (cd)") : qsTr("Abrir")
            onTriggered: {
                if (menu.targetIsDir) {
                    if (appState.activeTabId)
                        terminal.write(appState.activeTabId, "cd \"" + menu.targetPath + "\"\r")
                } else {
                    fileTree.openExternally(menu.targetPath)
                }
            }
        }
        MenuItem {
            text: qsTr("Abrir em programa padrão")
            visible: !menu.targetIsDir
            onTriggered: fileTree.openExternally(menu.targetPath)
        }
        MenuItem {
            text: qsTr("Inserir caminho no terminal")
            onTriggered: if (appState.activeTabId)
                terminal.write(appState.activeTabId, "\"" + menu.targetPath + "\"")
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Definir como raiz da árvore")
            visible: menu.targetIsDir
            onTriggered: fileTree.rootPath = menu.targetPath
        }
        MenuItem {
            text: qsTr("Revelar no Finder/Explorer")
            onTriggered: fileTree.revealInOsExplorer(menu.targetPath)
        }
        MenuItem {
            text: qsTr("Copiar caminho")
            onTriggered: fileTree.copyPath(menu.targetPath)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Nova pasta…")
            visible: menu.targetIsDir
            onTriggered: newDlg.openFor("folder", menu.targetPath)
        }
        MenuItem {
            text: qsTr("Novo arquivo…")
            visible: menu.targetIsDir
            onTriggered: newDlg.openFor("file", menu.targetPath)
        }
        MenuItem {
            text: qsTr("Renomear…")
            onTriggered: renameDlg.openFor(menu.targetPath)
        }
        MenuItem {
            text: qsTr("Duplicar")
            onTriggered: fileTree.duplicatePath(menu.targetPath)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Mover para lixeira")
            onTriggered: fileTree.deletePath(menu.targetPath)
        }
    }

    /* ─── New folder / file dialog ─── */
    Popup {
        id: newDlg
        modal: true
        focus: true
        anchors.centerIn: parent
        width: 340
        padding: 14
        property string mode: "folder"   // "folder" | "file"
        property string parentPath: ""

        function openFor(m, p) { mode = m; parentPath = p; nameField.text = ""; open() }
        background: Rectangle { color: Theme.surfaceHigh; border.color: Theme.borderStrong; radius: Theme.radiusMd }

        contentItem: ColumnLayout {
            spacing: 10
            Text {
                text: newDlg.mode === "folder" ? qsTr("Nova pasta") : qsTr("Novo arquivo")
                color: Theme.fgStrong
                font.family: Theme.fontSans
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            Text {
                text: qsTr("Em: %1").arg(newDlg.parentPath)
                color: Theme.fgFaint
                font.family: Theme.fontMono
                font.pixelSize: 10
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: newDlg.mode === "folder" ? qsTr("nome-da-pasta") : qsTr("arquivo.ext")
                color: Theme.fg
                selectByMouse: true
                onAccepted: confirmBtn.clicked()
                background: Rectangle { color: Theme.surfaceLow; border.color: Theme.border; radius: Theme.radiusSm }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancelar"); onClicked: newDlg.close() }
                Button {
                    id: confirmBtn
                    text: qsTr("Criar")
                    enabled: nameField.text.trim().length > 0
                    onClicked: {
                        if (newDlg.mode === "folder") fileTree.createFolder(newDlg.parentPath, nameField.text)
                        else                          fileTree.createFile(newDlg.parentPath, nameField.text)
                        newDlg.close()
                    }
                }
            }
        }
    }

    /* ─── Rename dialog ─── */
    Popup {
        id: renameDlg
        modal: true
        focus: true
        anchors.centerIn: parent
        width: 340
        padding: 14
        property string targetPath: ""

        function openFor(p) {
            targetPath = p
            // Pre-fill with current name.
            const parts = p.split("/")
            renameField.text = parts[parts.length - 1]
            open()
        }
        background: Rectangle { color: Theme.surfaceHigh; border.color: Theme.borderStrong; radius: Theme.radiusMd }

        contentItem: ColumnLayout {
            spacing: 10
            Text { text: qsTr("Renomear"); color: Theme.fgStrong; font.family: Theme.fontSans; font.pixelSize: 14; font.weight: Font.DemiBold }
            TextField {
                id: renameField
                Layout.fillWidth: true
                color: Theme.fg
                selectByMouse: true
                onAccepted: rConfirm.clicked()
                background: Rectangle { color: Theme.surfaceLow; border.color: Theme.border; radius: Theme.radiusSm }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancelar"); onClicked: renameDlg.close() }
                Button {
                    id: rConfirm
                    text: qsTr("Renomear")
                    enabled: renameField.text.trim().length > 0
                    onClicked: { fileTree.renamePath(renameDlg.targetPath, renameField.text); renameDlg.close() }
                }
            }
        }
    }

    /* ─── Helpers ─── */
    function iconForExtension(name) {
        const lc = (name || "").toLowerCase()
        if (lc.endsWith(".js") || lc.endsWith(".ts") || lc.endsWith(".tsx")) return "🟨"
        if (lc.endsWith(".py")) return "🐍"
        if (lc.endsWith(".rs")) return "🦀"
        if (lc.endsWith(".go")) return "🐹"
        if (lc.endsWith(".swift")) return "🦅"
        if (lc.endsWith(".md") || lc.endsWith(".markdown")) return "📝"
        if (lc.endsWith(".json")) return "{}"
        if (lc.endsWith(".yml") || lc.endsWith(".yaml")) return "⚙"
        if (lc.endsWith(".sh") || lc.endsWith(".zsh") || lc.endsWith(".bash")) return "💻"
        if (lc.endsWith(".png") || lc.endsWith(".jpg") || lc.endsWith(".jpeg") || lc.endsWith(".gif") || lc.endsWith(".webp")) return "🖼"
        if (lc.endsWith(".mp4") || lc.endsWith(".mov") || lc.endsWith(".m4v") || lc.endsWith(".avi") || lc.endsWith(".mkv") || lc.endsWith(".webm")) return "🎬"
        if (lc.endsWith(".mp3") || lc.endsWith(".wav") || lc.endsWith(".flac") || lc.endsWith(".m4a")) return "🎵"
        if (lc.endsWith(".pdf")) return "📕"
        if (lc.endsWith(".zip") || lc.endsWith(".tar") || lc.endsWith(".gz") || lc.endsWith(".7z")) return "🗜"
        return "📄"
    }

    component IconBtn: Rectangle {
        property string glyph
        property string tip
        signal clicked()
        implicitWidth: 26; implicitHeight: 22
        radius: 4
        color: btnMa.containsMouse ? Theme.surfaceTop : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
        Text { anchors.centerIn: parent; text: glyph; color: Theme.fgMuted; font.pixelSize: 11 }
        MouseArea {
            id: btnMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
            ToolTip.text: tip
            ToolTip.visible: containsMouse && tip.length > 0
            ToolTip.delay: 400
        }
    }
}
