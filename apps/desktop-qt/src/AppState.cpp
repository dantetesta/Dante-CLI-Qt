#include "AppState.h"
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include <algorithm>

namespace dante {

namespace {
    QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

    int indexOfTab(const QVector<Tab>& tabs, const QString& id) {
        for (int i = 0; i < tabs.size(); ++i) if (tabs[i].id == id) return i;
        return -1;
    }

    QJsonObject spansToJson(const QVariantMap& m) {
        QJsonObject o;
        for (auto it = m.constBegin(); it != m.constEnd(); ++it) {
            const auto s = it.value().toMap();
            int sc = s.value("cols").toInt(); if (sc <= 0) sc = 1;
            int sr = s.value("rows").toInt(); if (sr <= 0) sr = 1;
            o.insert(it.key(), QJsonObject{ {"cols", sc}, {"rows", sr} });
        }
        return o;
    }
    QVariantMap spansFromJson(const QJsonObject& o) {
        QVariantMap m;
        for (auto it = o.constBegin(); it != o.constEnd(); ++it) {
            const auto s = it.value().toObject();
            m.insert(it.key(), QVariantMap{
                {"cols", s.value("cols").toInt(1)},
                {"rows", s.value("rows").toInt(1)},
            });
        }
        return m;
    }

    // SPEC-110 — recursive serialization for the pane tree.
    QJsonValue paneTreeToJson(const QVariantMap& node) {
        if (node.isEmpty()) return QJsonValue();
        if (node.contains("leaf")) {
            return QJsonObject{{"leaf", node.value("leaf").toString()}};
        }
        bool ok = false;
        double r = node.value("ratio").toDouble(&ok);
        if (!ok) r = 0.5;
        return QJsonObject{
            {"split", node.value("split").toString()},
            {"ratio", r},
            {"first",  paneTreeToJson(node.value("first").toMap())},
            {"second", paneTreeToJson(node.value("second").toMap())},
        };
    }
    QVariantMap paneTreeFromJson(const QJsonValue& v) {
        if (!v.isObject()) return {};
        const auto o = v.toObject();
        if (o.contains("leaf")) {
            return { {"leaf", o.value("leaf").toString()} };
        }
        return {
            {"split",  o.value("split").toString()},
            {"ratio",  o.value("ratio").toDouble(0.5)},
            {"first",  paneTreeFromJson(o.value("first"))},
            {"second", paneTreeFromJson(o.value("second"))},
        };
    }

    /// Returns true if `node` contains a leaf with the given sessionId.
    bool paneTreeContains(const QVariantMap& node, const QString& sid) {
        if (node.isEmpty()) return false;
        if (node.contains("leaf")) return node.value("leaf").toString() == sid;
        return paneTreeContains(node.value("first").toMap(), sid)
            || paneTreeContains(node.value("second").toMap(), sid);
    }

    /// Returns every leaf sessionId in DFS order.
    QStringList paneTreeLeaves(const QVariantMap& node) {
        QStringList out;
        if (node.isEmpty()) return out;
        if (node.contains("leaf")) { out << node.value("leaf").toString(); return out; }
        out << paneTreeLeaves(node.value("first").toMap());
        out << paneTreeLeaves(node.value("second").toMap());
        return out;
    }

    /// Returns a copy of `node` with the leaf matching `target` replaced by
    /// a new split holding (oldLeaf, newLeaf) in the given direction.
    QVariantMap paneTreeSplit(const QVariantMap& node, const QString& target,
                              const QString& axis, const QString& newLeaf) {
        if (node.isEmpty()) return node;
        if (node.contains("leaf")) {
            if (node.value("leaf").toString() != target) return node;
            return {
                {"split", axis},
                {"ratio", 0.5},
                {"first",  QVariantMap{{"leaf", target}}},
                {"second", QVariantMap{{"leaf", newLeaf}}},
            };
        }
        QVariantMap copy = node;
        copy["first"]  = paneTreeSplit(node.value("first").toMap(),  target, axis, newLeaf);
        copy["second"] = paneTreeSplit(node.value("second").toMap(), target, axis, newLeaf);
        return copy;
    }

    /// Returns the node with `target` removed and its sibling collapsed up.
    /// If the whole tree degenerates to a single leaf, returns that leaf
    /// (caller can detect and store as Tab.sessionId).
    QVariantMap paneTreeRemove(const QVariantMap& node, const QString& target) {
        if (node.isEmpty()) return node;
        if (node.contains("leaf")) {
            return (node.value("leaf").toString() == target) ? QVariantMap{} : node;
        }
        const auto first  = node.value("first").toMap();
        const auto second = node.value("second").toMap();
        // Direct hit on either side → collapse to the other.
        if (first.contains("leaf")  && first.value("leaf").toString()  == target) return second;
        if (second.contains("leaf") && second.value("leaf").toString() == target) return first;
        QVariantMap copy = node;
        if (paneTreeContains(first, target)) {
            const auto pruned = paneTreeRemove(first, target);
            if (pruned.isEmpty()) return second;
            copy["first"] = pruned;
            return copy;
        }
        if (paneTreeContains(second, target)) {
            const auto pruned = paneTreeRemove(second, target);
            if (pruned.isEmpty()) return first;
            copy["second"] = pruned;
            return copy;
        }
        return node;
    }

    QJsonObject tabToJson(const Tab& t) {
        return {
            {"id", t.id}, {"title", t.title}, {"color", t.color},
            {"emoji", t.emoji}, {"pinned", t.pinned},
            {"kind", int(t.kind)}, {"cwd", t.cwd},
            {"sessionId", t.sessionId},
            {"splitMode", t.splitMode},
            {"secondSessionId", t.secondSessionId},
            {"splitFraction", t.splitFraction},
            {"gridCols", t.gridCols},
            {"gridRows", t.gridRows},
            {"gridSpans", spansToJson(t.gridSpans)},
            {"editorPath", t.editorPath},
            // editorContent is NOT persisted — too heavy for session.json on
            // big files. We persist editorPath + editorDirty; on launch a
            // dirty editor reloads from disk and shows a "modified" hint.
            {"editorLanguage", t.editorLanguage},
            {"editorDirty", t.editorDirty},
            {"paneTree", paneTreeToJson(t.paneTree)},
            {"videoPath", t.videoPath},
            {"browserUrl", t.browserUrl},
        };
    }
    Tab tabFromJson(const QJsonObject& o) {
        Tab t;
        t.id = o.value("id").toString();
        t.title = o.value("title").toString("Terminal");
        t.color = o.value("color").toString("#0A84FF");
        t.emoji = o.value("emoji").toString("💻");
        t.pinned = o.value("pinned").toBool();
        t.kind = TabKind(o.value("kind").toInt(int(TabKind::Terminal)));
        t.cwd = o.value("cwd").toString();
        t.sessionId = o.value("sessionId").toString();
        t.splitMode = o.value("splitMode").toString();
        t.secondSessionId = o.value("secondSessionId").toString();
        t.splitFraction = o.value("splitFraction").toDouble(0.5);
        t.gridCols = o.value("gridCols").toInt(0);
        t.gridRows = o.value("gridRows").toInt(0);
        t.gridSpans = spansFromJson(o.value("gridSpans").toObject());
        t.editorPath = o.value("editorPath").toString();
        t.editorLanguage = o.value("editorLanguage").toString();
        t.editorDirty = o.value("editorDirty").toBool();
        t.paneTree = paneTreeFromJson(o.value("paneTree"));
        t.videoPath = o.value("videoPath").toString();
        t.browserUrl = o.value("browserUrl").toString();
        return t;
    }
}

AppState::AppState(QObject* parent)
    : QObject(parent)
    , sessionStore_(std::make_unique<persistence::JsonStore>("session.json", 300, this))
    , settingsStore_(std::make_unique<persistence::JsonStore>("settings.json", 500, this))
{}

void AppState::hydrate() {
    // settings.json
    {
        const auto doc = settingsStore_->read({});
        const auto o = doc.object();
        if (!o.isEmpty()) {
            settings_.groqApiKey         = o.value("groqApiKey").toString();
            settings_.terminalScheme     = o.value("terminalScheme").toString("TokyoNight");
            settings_.fontName           = o.value("fontName").toString("JetBrains Mono");
            settings_.fontSize           = o.value("fontSize").toInt(13);
            settings_.scrollback         = o.value("scrollback").toInt(50000);
            settings_.sidebarWidth       = o.value("sidebarWidth").toInt(280);
            settings_.sidebarMode        = SidebarMode(o.value("sidebarMode").toInt(0));
            settings_.rightSidebarVisible= o.value("rightSidebarVisible").toBool(false);
            settings_.shellOverride      = o.value("shellOverride").toString();
            settings_.restoreOnLaunch    = o.value("restoreOnLaunch").toBool(true);
            settings_.voiceLanguage      = o.value("voiceLanguage").toString("pt");
            settings_.voiceModel         = o.value("voiceModel").toString("whisper-large-v3-turbo");
            settings_.voiceAutoSubmit    = o.value("voiceAutoSubmit").toBool(false);
            settings_.appearance         = AppearanceMode(o.value("appearance").toInt(int(AppearanceMode::Dark)));
            settings_.autoCheckUpdates   = o.value("autoCheckUpdates").toBool(true);
            const auto recents = o.value("recentEmojis").toArray();
            if (!recents.isEmpty()) {
                settings_.recentEmojis.clear();
                for (const auto& v : recents) settings_.recentEmojis.append(v.toString());
            }
            settings_.uiLanguage = o.value("uiLanguage").toString("system");
        }
    }
    emit settingsChanged();
    emit sidebarWidthChanged();
    emit sidebarModeChanged();
    emit rightSidebarVisibleChanged();

    // session.json
    {
        const auto doc = sessionStore_->read({});
        const auto o = doc.object();
        const auto arr = o.value("tabs").toArray();
        tabs_.clear();
        for (const auto& v : arr) tabs_.append(tabFromJson(v.toObject()));
        activeTabId_ = o.value("activeTabId").toString();
    }
    if (tabs_.isEmpty()) {
        Tab t;
        t.id = newId();
        t.title = "Terminal";
        t.sessionId = newId();
        tabs_.append(t);
        activeTabId_ = t.id;
    } else if (activeTabId_.isEmpty()) {
        activeTabId_ = tabs_.first().id;
    }
    emit tabsChanged();
    emit activeTabIdChanged();
}

void AppState::saveOnClose() {
    persistSession();
    persistSettings();
    sessionStore_->flush();
    settingsStore_->flush();
}

QString AppState::addTab(QString title) {
    Tab t;
    t.id = newId();
    t.title = std::move(title);
    t.sessionId = newId();
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}

void AppState::closeTab(const QString& id) {
    int idx = -1;
    for (int i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].id == id) { idx = i; break; }
    }
    if (idx < 0) return;
    // SPEC-120 / 091 — pinned tabs are immune to closeTab. The user has to
    // explicitly unpin from the right-click menu (mirrors Swift behaviour).
    if (tabs_[idx].pinned) return;
    tabs_.removeAt(idx);
    if (tabs_.isEmpty()) {
        // Emergency new tab.
        Tab t;
        t.id = newId();
        t.sessionId = newId();
        tabs_.append(t);
        activeTabId_ = t.id;
    } else if (activeTabId_ == id) {
        activeTabId_ = tabs_[qMax(0, idx - 1)].id;
    }
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
}

void AppState::selectTab(const QString& id) { setActiveTabId(id); }

/* ─── SPEC-120 — tab-chip mutations ─────────────────────────────────────── */

void AppState::setTabTitle(const QString& tabId, const QString& title) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    const QString clean = title.trimmed().isEmpty() ? QStringLiteral("Terminal")
                                                    : title.trimmed();
    if (tabs_[i].title == clean) return;
    tabs_[i].title = clean;
    emit tabsChanged();
    persistSession();
}

void AppState::setTabColor(const QString& tabId, const QString& colorHex) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    if (tabs_[i].color == colorHex) return;
    tabs_[i].color = colorHex;
    emit tabsChanged();
    persistSession();
}

void AppState::setTabEmoji(const QString& tabId, const QString& emoji) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    if (tabs_[i].emoji == emoji) return;
    tabs_[i].emoji = emoji;
    emit tabsChanged();
    persistSession();
}

void AppState::setTabScheme(const QString& tabId, const QString& schemeId) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    if (tabs_[i].customScheme == schemeId) return;
    tabs_[i].customScheme = schemeId;
    emit tabsChanged();
    persistSession();
}

void AppState::setTabPinned(const QString& tabId, bool pinned) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    if (tabs_[i].pinned == pinned) return;
    tabs_[i].pinned = pinned;
    emit tabsChanged();
    persistSession();
}

bool AppState::isTabPinned(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i >= 0 ? tabs_[i].pinned : false;
}

/* ─── SPEC-021 — Editor tab kind ─────────────────────────────────────── */

QString AppState::openFileInEditor(const QString& path) {
    const QString abs = QFileInfo(path).absoluteFilePath();
    // Re-use an existing editor tab pointing at the same path.
    for (const auto& t : tabs_) {
        if (t.kind == TabKind::Editor && t.editorPath == abs) {
            activeTabId_ = t.id;
            emit activeTabIdChanged();
            return t.id;
        }
    }
    QFile f(abs);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote() << "[editor] open failed:" << f.errorString();
        return {};
    }
    // 5 MB ceiling — anything bigger we refuse rather than freeze the UI.
    if (f.size() > 5 * 1024 * 1024) {
        qWarning().noquote() << "[editor] file too large:" << f.size();
        return {};
    }
    const QString content = QString::fromUtf8(f.readAll());
    f.close();

    Tab t;
    t.id = newId();
    t.kind = TabKind::Editor;
    t.title = QFileInfo(abs).fileName();
    t.emoji = QStringLiteral("📝");
    t.color = QStringLiteral("#F59E0B");   // amber differentiates from terminal
    t.editorPath = abs;
    t.editorContent = content;
    t.editorDirty = false;
    // Language hint from extension. EditorView uses this to pick syntax rules
    // (highlighter lands as a follow-up, but the field already persists).
    t.editorLanguage = QFileInfo(abs).suffix().toLower();
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}

bool AppState::saveEditor(const QString& tabId) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return false;
    Tab& t = tabs_[i];
    if (t.kind != TabKind::Editor || t.editorPath.isEmpty()) return false;

    QFile f(t.editorPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning().noquote() << "[editor] save failed:" << f.errorString();
        return false;
    }
    f.write(t.editorContent.toUtf8());
    f.close();

    t.editorDirty = false;
    emit tabsChanged();
    persistSession();
    return true;
}

void AppState::setEditorContent(const QString& tabId, const QString& content) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    Tab& t = tabs_[i];
    if (t.kind != TabKind::Editor) return;
    if (t.editorContent == content) return;
    t.editorContent = content;
    if (!t.editorDirty) {
        t.editorDirty = true;
        emit tabsChanged();   // refresh chip "● modified" indicator
    }
    // Don't persist on every keystroke — only on save/saveOnClose.
}

QString AppState::editorPath(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].editorPath;
}
QString AppState::editorContent(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].editorContent;
}
QString AppState::editorLanguage(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].editorLanguage;
}
bool AppState::editorDirty(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i >= 0 && tabs_[i].editorDirty;
}
int AppState::tabKind(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? 0 : int(tabs_[i].kind);
}
QString AppState::tabKindString(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return QStringLiteral("terminal");
    switch (tabs_[i].kind) {
        case TabKind::Terminal:   return QStringLiteral("terminal");
        case TabKind::Editor:     return QStringLiteral("editor");
        case TabKind::Preview:    return QStringLiteral("preview");
        case TabKind::Video:      return QStringLiteral("video");
        case TabKind::Browser:    return QStringLiteral("browser");
        case TabKind::Calculator: return QStringLiteral("calculator");
    }
    return QStringLiteral("terminal");
}
QString AppState::newVideoTab(const QString& title, const QString& path) {
    Tab t;
    t.id        = newId();
    t.kind      = TabKind::Video;
    t.title     = title.isEmpty() ? QStringLiteral("Vídeo") : title;
    t.emoji     = QStringLiteral("🎬");
    t.color     = QStringLiteral("#7C82FF");
    t.videoPath = path;
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}
QString AppState::newBrowserTab(const QString& title, const QString& url) {
    Tab t;
    t.id         = newId();
    t.kind       = TabKind::Browser;
    t.title      = title.isEmpty() ? QStringLiteral("Navegador") : title;
    t.emoji      = QStringLiteral("🌐");
    t.color      = QStringLiteral("#7C82FF");
    t.browserUrl = url;
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}
QString AppState::tabBrowserUrl(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].browserUrl;
}
void AppState::setTabBrowserUrl(const QString& tabId, const QString& url) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    Tab& t = tabs_[i];
    if (t.browserUrl == url) return;
    t.browserUrl = url;
    emit tabsChanged();
    persistSession();
}
QString AppState::tabVideoPath(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].videoPath;
}
void AppState::setTabVideoPath(const QString& tabId, const QString& path) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    Tab& t = tabs_[i];
    if (t.videoPath == path) return;
    t.videoPath = path;
    emit tabsChanged();
    persistSession();
}
QString AppState::openCalculatorTab() {
    for (const auto& t : tabs_) {
        if (t.kind == TabKind::Calculator) {
            activeTabId_ = t.id;
            emit activeTabIdChanged();
            return t.id;
        }
    }
    Tab t;
    t.id    = newId();
    t.kind  = TabKind::Calculator;
    t.title = QStringLiteral("Calculadora");
    t.emoji = QStringLiteral("🧮");
    t.color = QStringLiteral("#7C82FF");
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}

/* ─────────────────────────────────────────────────────────────────────── */

QString AppState::duplicateTab(const QString& tabId) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return {};
    Tab t = tabs_[i];          // copy metadata
    t.id = newId();            // fresh ids — no shared PTY
    t.sessionId = newId();
    t.secondSessionId.clear();
    t.splitMode.clear();
    t.gridCols = 0;
    t.gridRows = 0;
    t.gridSpans.clear();
    t.pinned = false;          // duplicates start unpinned by convention
    tabs_.append(t);
    activeTabId_ = t.id;
    emit tabsChanged();
    emit activeTabIdChanged();
    persistSession();
    return t.id;
}

void AppState::setActiveTabId(QString v) {
    if (activeTabId_ == v) return;
    activeTabId_ = std::move(v);
    emit activeTabIdChanged();
    persistSession();
}

void AppState::setFilter(QString v) {
    if (filter_ == v) return;
    filter_ = std::move(v);
    emit filterChanged();
}

void AppState::setSidebarWidth(int v) {
    v = qBound(220, v, 560);
    if (settings_.sidebarWidth == v) return;
    settings_.sidebarWidth = v;
    emit sidebarWidthChanged();
    persistSettings();
}
void AppState::setSidebarMode(int v) {
    const auto m = SidebarMode(qBound(0, v, 3));
    if (settings_.sidebarMode == m) return;
    settings_.sidebarMode = m;
    emit sidebarModeChanged();
    persistSettings();
}
void AppState::setRightSidebarVisible(bool v) {
    if (settings_.rightSidebarVisible == v) return;
    settings_.rightSidebarVisible = v;
    emit rightSidebarVisibleChanged();
    persistSettings();
}
void AppState::setGroqApiKey(QString v)     { settings_.groqApiKey = std::move(v);     emit settingsChanged(); persistSettings(); }
void AppState::setTerminalScheme(QString v) { settings_.terminalScheme = std::move(v); emit settingsChanged(); persistSettings(); }
void AppState::setFontName(QString v)       { settings_.fontName = std::move(v);       emit settingsChanged(); persistSettings(); }
void AppState::setFontSize(int v)           { settings_.fontSize = v;                  emit settingsChanged(); persistSettings(); }
void AppState::setShellOverride(QString v)  { settings_.shellOverride = std::move(v);  emit settingsChanged(); persistSettings(); }
void AppState::setScrollback(int v)         { settings_.scrollback = qMax(1000, v);    emit settingsChanged(); persistSettings(); }
void AppState::setRestoreOnLaunch(bool v)   { settings_.restoreOnLaunch = v;           emit settingsChanged(); persistSettings(); }
void AppState::setVoiceLanguage(QString v)  { settings_.voiceLanguage = std::move(v);  emit settingsChanged(); persistSettings(); }
void AppState::setVoiceModel(QString v)     { settings_.voiceModel = std::move(v);     emit settingsChanged(); persistSettings(); }
void AppState::setVoiceAutoSubmit(bool v)   { settings_.voiceAutoSubmit = v;           emit settingsChanged(); persistSettings(); }
void AppState::setAppearanceMode(int v)     { settings_.appearance = AppearanceMode(qBound(0, v, 2)); emit settingsChanged(); persistSettings(); }
void AppState::setAutoCheckUpdates(bool v)  { settings_.autoCheckUpdates = v;          emit settingsChanged(); persistSettings(); }

void AppState::pushRecentEmoji(const QString& emoji) {
    const QString e = emoji.trimmed();
    if (e.isEmpty()) return;
    auto& list = settings_.recentEmojis;
    list.removeAll(e);
    list.prepend(e);
    while (list.size() > 32) list.removeLast();
    emit settingsChanged();
    persistSettings();
}

/* ─── Split panes ─────────────────────────────────────────────────────────── */

QString AppState::tabSplitMode(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].splitMode;
}

QString AppState::tabPrimarySessionId(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].sessionId;
}

QString AppState::tabSecondSessionId(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].secondSessionId;
}

double AppState::tabSplitFraction(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? 0.5 : tabs_[i].splitFraction;
}

void AppState::setTabSplitFraction(const QString& tabId, double f) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    f = std::clamp(f, 0.05, 0.95);
    if (qFuzzyCompare(tabs_[i].splitFraction, f)) return;
    tabs_[i].splitFraction = f;
    persistSession();
}

int AppState::tabGridCols(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? 0 : tabs_[i].gridCols;
}
int AppState::tabGridRows(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? 0 : tabs_[i].gridRows;
}
QVariantMap AppState::tabGridSpans(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QVariantMap() : tabs_[i].gridSpans;
}

/* ─── SPEC-110 — recursive pane tree ─────────────────────────────────────── */

QVariantMap AppState::tabPaneTree(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QVariantMap() : tabs_[i].paneTree;
}

QString AppState::splitPane(const QString& tabId,
                            const QString& targetSessionId,
                            const QString& axis) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return {};
    if (axis != "vertical" && axis != "horizontal") return {};

    Tab& t = tabs_[i];
    const QString newSid = newId();

    // First split: promote to a tree if there isn't one yet.
    if (t.paneTree.isEmpty()) {
        // Bootstrap from Tab.sessionId — but the caller passes a sessionId
        // that should match. If they don't match, prefer the caller's
        // because that's what the QML focused-pane heuristic uses.
        const QString anchor = targetSessionId.isEmpty() ? t.sessionId : targetSessionId;
        if (t.sessionId.isEmpty()) t.sessionId = anchor;
        t.paneTree = {
            {"split", axis},
            {"ratio", 0.5},
            {"first",  QVariantMap{{"leaf", anchor}}},
            {"second", QVariantMap{{"leaf", newSid}}},
        };
    } else {
        t.paneTree = paneTreeSplit(t.paneTree, targetSessionId, axis, newSid);
    }
    // Clear legacy 2-pane fields so they don't double-render.
    t.splitMode.clear();
    t.secondSessionId.clear();
    emit tabSplitChanged(t.id);
    persistSession();
    return newSid;
}

void AppState::closePaneInTree(const QString& tabId,
                               const QString& targetSessionId) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0 || tabs_[i].paneTree.isEmpty()) return;
    Tab& t = tabs_[i];
    const auto next = paneTreeRemove(t.paneTree, targetSessionId);

    // Tree degenerated to a single leaf → store as Tab.sessionId + clear tree.
    if (next.contains("leaf")) {
        t.sessionId = next.value("leaf").toString();
        t.paneTree.clear();
    } else if (next.isEmpty()) {
        // Whole tree removed — nothing left. Fall back to a fresh session.
        t.sessionId = newId();
        t.paneTree.clear();
    } else {
        t.paneTree = next;
    }
    emit tabSplitChanged(t.id);
    persistSession();
}

void AppState::setPaneRatio(const QString& tabId,
                            const QVariantList& path,
                            double ratio) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0 || tabs_[i].paneTree.isEmpty()) return;
    ratio = std::clamp(ratio, 0.05, 0.95);
    Tab& t = tabs_[i];

    // Walk the path, copy-on-write. Recursive lambda since QVariantMap is
    // a value type — we rebuild the spine of the tree.
    std::function<QVariantMap(const QVariantMap&, int)> walk =
        [&](const QVariantMap& node, int depth) -> QVariantMap {
        if (depth >= path.size() || node.contains("leaf")) {
            // Reached the target node — overwrite ratio.
            if (node.contains("split")) {
                QVariantMap copy = node;
                copy["ratio"] = ratio;
                return copy;
            }
            return node;
        }
        const int step = path.at(depth).toInt();
        QVariantMap copy = node;
        if (step == 0) {
            copy["first"]  = walk(node.value("first").toMap(),  depth + 1);
        } else {
            copy["second"] = walk(node.value("second").toMap(), depth + 1);
        }
        return copy;
    };
    t.paneTree = walk(t.paneTree, 0);
    // Don't fire tabSplitChanged — the QML splitter is dragging and re-emit
    // would tear down the tree. Just persist on release (caller calls
    // setPaneRatio once on onReleased — matches SplitContainer behavior).
    persistSession();
}

void AppState::setTabGrid(const QString& tabId, int cols, int rows, const QVariantMap& spans) {
    const int i = indexOfTab(tabs_, tabId);
    if (i < 0) return;
    Tab& t = tabs_[i];
    // cols/rows == 0 → clear grid, restore single-pane.
    if (cols <= 0 || rows <= 0) {
        if (t.gridCols == 0 && t.gridRows == 0) return;
        t.gridCols = 0;
        t.gridRows = 0;
        t.gridSpans.clear();
        emit tabSplitChanged(t.id);
        persistSession();
        return;
    }
    cols = qBound(1, cols, 6);
    rows = qBound(1, rows, 6);
    // Switching INTO grid mode also clears the 2-pane state so the two
    // worlds never conflict.
    t.splitMode.clear();
    t.secondSessionId.clear();
    t.gridCols = cols;
    t.gridRows = rows;
    t.gridSpans = spans;
    emit tabSplitChanged(t.id);
    persistSession();
}

void AppState::splitActive(const QString& direction) {
    const int i = indexOfTab(tabs_, activeTabId_);
    if (i < 0) return;
    Tab& t = tabs_[i];

    if (direction.isEmpty()) {
        // Collapse: keep "a", drop "b". Caller (QML) is expected to have
        // already killed the b-session's PTY via `terminal.kill(id)`.
        if (t.splitMode.isEmpty()) return;
        t.splitMode.clear();
        t.secondSessionId.clear();
        emit tabSplitChanged(t.id);
        persistSession();
        return;
    }
    if (direction != "vertical" && direction != "horizontal") return;

    // UUID-stable: ":b" suffix means the same secondary session survives
    // toggles between vertical/horizontal split without respawning.
    if (t.secondSessionId.isEmpty()) {
        t.secondSessionId = t.sessionId.isEmpty()
            ? newId() + ":b"
            : t.sessionId + ":b";
    }
    t.splitMode = direction;
    emit tabSplitChanged(t.id);
    persistSession();
}

void AppState::closeActivePane(const QString& side) {
    const int i = indexOfTab(tabs_, activeTabId_);
    if (i < 0) return;
    Tab& t = tabs_[i];
    if (t.splitMode.isEmpty()) return;

    if (side == "b") {
        // Drop the right/bottom pane. PTY kill happens on QML side.
        t.splitMode.clear();
        t.secondSessionId.clear();
    } else if (side == "a") {
        // Promote b to a: the QML side kills the *old* a-session, then
        // re-keys what was b onto the primary slot.
        t.sessionId = t.secondSessionId;
        t.splitMode.clear();
        t.secondSessionId.clear();
    } else {
        return;
    }
    emit tabSplitChanged(t.id);
    persistSession();
}

void AppState::persistSession() {
    QJsonArray arr;
    for (const auto& t : tabs_) arr.append(tabToJson(t));
    sessionStore_->scheduleWrite(QJsonDocument(QJsonObject{
        {"tabs", arr}, {"activeTabId", activeTabId_},
    }));
}

void AppState::persistSettings() {
    settingsStore_->scheduleWrite(QJsonDocument(QJsonObject{
        {"groqApiKey",          settings_.groqApiKey},
        {"terminalScheme",      settings_.terminalScheme},
        {"fontName",            settings_.fontName},
        {"fontSize",            settings_.fontSize},
        {"scrollback",          settings_.scrollback},
        {"sidebarWidth",        settings_.sidebarWidth},
        {"sidebarMode",         int(settings_.sidebarMode)},
        {"rightSidebarVisible", settings_.rightSidebarVisible},
        {"shellOverride",       settings_.shellOverride},
        {"restoreOnLaunch",     settings_.restoreOnLaunch},
        {"voiceLanguage",       settings_.voiceLanguage},
        {"voiceModel",          settings_.voiceModel},
        {"voiceAutoSubmit",     settings_.voiceAutoSubmit},
        {"appearance",          int(settings_.appearance)},
        {"autoCheckUpdates",    settings_.autoCheckUpdates},
        {"recentEmojis",        QJsonArray::fromStringList(settings_.recentEmojis)},
        {"uiLanguage",          settings_.uiLanguage},
    }));
}

QString AppState::uiLanguage() const { return settings_.uiLanguage; }
void AppState::setUiLanguage(const QString& locale) {
    if (settings_.uiLanguage == locale) return;
    settings_.uiLanguage = locale.isEmpty() ? QStringLiteral("system") : locale;
    persistSettings();
    emit settingsChanged();
}

/* ─── SPEC-170 — Drag tab → grid slot ─── */

void AppState::attachTabToSlot(const QString& sourceTabId,
                               const QString& hostTabId,
                               int cellIndex) {
    if (sourceTabId.isEmpty() || hostTabId.isEmpty() || cellIndex < 0) return;
    if (sourceTabId == hostTabId) return;
    // Clear any previous assignment of this source on any host (1 tab → 1 slot).
    for (auto& t : tabs_) {
        QVariantMap spans = t.gridSpans;
        bool changed = false;
        for (auto it = spans.begin(); it != spans.end(); ++it) {
            auto m = it.value().toMap();
            if (m.value("tabId").toString() == sourceTabId) {
                m.remove("tabId");
                it.value() = m;
                changed = true;
            }
        }
        if (changed) {
            t.gridSpans = spans;
            emit tabSplitChanged(t.id);
        }
    }
    const int hi = indexOfTab(tabs_, hostTabId);
    if (hi < 0) return;
    Tab& host = tabs_[hi];
    QVariantMap spans = host.gridSpans;
    const QString key = QString::number(cellIndex);
    QVariantMap m = spans.value(key).toMap();
    m["tabId"] = sourceTabId;
    if (!m.contains("cols")) m["cols"] = 1;
    if (!m.contains("rows")) m["rows"] = 1;
    spans[key] = m;
    host.gridSpans = spans;
    emit tabSplitChanged(host.id);
    persistSession();
}

QString AppState::tabAtSlot(const QString& hostTabId, int cellIndex) const {
    const int i = indexOfTab(tabs_, hostTabId);
    if (i < 0 || cellIndex < 0) return {};
    const auto m = tabs_[i].gridSpans.value(QString::number(cellIndex)).toMap();
    return m.value("tabId").toString();
}

void AppState::detachTabFromSlot(const QString& hostTabId, int cellIndex) {
    const int i = indexOfTab(tabs_, hostTabId);
    if (i < 0 || cellIndex < 0) return;
    Tab& host = tabs_[i];
    QVariantMap spans = host.gridSpans;
    const QString key = QString::number(cellIndex);
    auto m = spans.value(key).toMap();
    if (!m.contains("tabId")) return;
    m.remove("tabId");
    spans[key] = m;
    host.gridSpans = spans;
    emit tabSplitChanged(host.id);
    persistSession();
}

QString AppState::tabTitle(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].title;
}
QString AppState::tabEmoji(const QString& tabId) const {
    const int i = indexOfTab(tabs_, tabId);
    return i < 0 ? QString() : tabs_[i].emoji;
}

} // namespace dante
