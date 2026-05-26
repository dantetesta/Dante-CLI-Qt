#include "AppState.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace dante {

namespace {
    QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

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

/* ─── Split panes ─────────────────────────────────────────────────────────── */

namespace {
    int indexOfTab(const QVector<Tab>& tabs, const QString& id) {
        for (int i = 0; i < tabs.size(); ++i) if (tabs[i].id == id) return i;
        return -1;
    }
}

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
    }));
}

} // namespace dante
