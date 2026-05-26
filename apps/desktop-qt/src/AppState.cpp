#include "AppState.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace dante {

namespace {
    QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

    QJsonObject tabToJson(const Tab& t) {
        return {
            {"id", t.id}, {"title", t.title}, {"color", t.color},
            {"emoji", t.emoji}, {"pinned", t.pinned},
            {"kind", int(t.kind)}, {"cwd", t.cwd},
            {"sessionId", t.sessionId},
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
    }));
}

} // namespace dante
