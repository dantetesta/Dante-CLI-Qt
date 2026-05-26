// Central app state — analogous to `AppState.swift`.
// Exposed to QML via context property "appState".
#pragma once

#include "domain/Models.h"
#include "persistence/JsonStore.h"
#include <QObject>
#include <QVector>
#include <memory>

namespace dante {

class AppState : public QObject {
    Q_OBJECT
    Q_PROPERTY(int     sidebarWidth        READ sidebarWidth        WRITE setSidebarWidth        NOTIFY sidebarWidthChanged)
    Q_PROPERTY(int     sidebarMode         READ sidebarMode         WRITE setSidebarMode         NOTIFY sidebarModeChanged)
    Q_PROPERTY(bool    rightSidebarVisible READ rightSidebarVisible WRITE setRightSidebarVisible NOTIFY rightSidebarVisibleChanged)
    Q_PROPERTY(QString activeTabId         READ activeTabId         WRITE setActiveTabId         NOTIFY activeTabIdChanged)
    Q_PROPERTY(QString filter              READ filter              WRITE setFilter              NOTIFY filterChanged)
    Q_PROPERTY(QString groqApiKey          READ groqApiKey          WRITE setGroqApiKey          NOTIFY settingsChanged)
    Q_PROPERTY(QString terminalScheme      READ terminalScheme      WRITE setTerminalScheme      NOTIFY settingsChanged)
    Q_PROPERTY(QString fontName            READ fontName            WRITE setFontName            NOTIFY settingsChanged)
    Q_PROPERTY(int     fontSize            READ fontSize            WRITE setFontSize            NOTIFY settingsChanged)
    Q_PROPERTY(QString shellOverride       READ shellOverride       WRITE setShellOverride       NOTIFY settingsChanged)
    Q_PROPERTY(int     scrollback          READ scrollback          WRITE setScrollback          NOTIFY settingsChanged)
    Q_PROPERTY(bool    restoreOnLaunch     READ restoreOnLaunch     WRITE setRestoreOnLaunch     NOTIFY settingsChanged)
    Q_PROPERTY(QString voiceLanguage       READ voiceLanguage       WRITE setVoiceLanguage       NOTIFY settingsChanged)
    Q_PROPERTY(QString voiceModel          READ voiceModel          WRITE setVoiceModel          NOTIFY settingsChanged)
    Q_PROPERTY(bool    voiceAutoSubmit     READ voiceAutoSubmit     WRITE setVoiceAutoSubmit     NOTIFY settingsChanged)
    Q_PROPERTY(int     appearanceMode      READ appearanceMode      WRITE setAppearanceMode      NOTIFY settingsChanged)
    Q_PROPERTY(bool    autoCheckUpdates    READ autoCheckUpdates    WRITE setAutoCheckUpdates    NOTIFY settingsChanged)
public:
    explicit AppState(QObject* parent = nullptr);

    void hydrate();
    Q_INVOKABLE void saveOnClose();

    /* ─── Tabs ─── */
    QVector<Tab>& tabsRef() { return tabs_; }
    const QVector<Tab>& tabs() const { return tabs_; }
    Q_INVOKABLE QString addTab(QString title = "Terminal");
    Q_INVOKABLE void closeTab(const QString& id);
    Q_INVOKABLE void selectTab(const QString& id);

    /* ─── Split panes (max 2 per tab) ─── */
    /// "" / "vertical" / "horizontal".
    Q_INVOKABLE QString tabSplitMode(const QString& tabId) const;
    /// First-pane session id (always == tab.sessionId).
    Q_INVOKABLE QString tabPrimarySessionId(const QString& tabId) const;
    /// Second-pane session id (empty when not split).
    Q_INVOKABLE QString tabSecondSessionId(const QString& tabId) const;
    /// Splits the active tab. `direction` ∈ {"vertical","horizontal",""}.
    /// Empty string collapses back to a single pane (closes the b-pane).
    Q_INVOKABLE void splitActive(const QString& direction);
    /// Close one side of the split on the active tab. `side` ∈ {"a","b"}.
    /// If "a" is closed, the b-session becomes the sole pane (re-keyed).
    Q_INVOKABLE void closeActivePane(const QString& side);
    /// Current split-fraction (0..1) for the given tab. Default 0.5.
    Q_INVOKABLE double tabSplitFraction(const QString& tabId) const;
    /// Persist a new split fraction. Clamped to [0.05, 0.95] to avoid
    /// collapsing a pane to invisibility from a frantic drag.
    Q_INVOKABLE void setTabSplitFraction(const QString& tabId, double f);

    /* ─── Settings ─── */
    QString groqApiKey() const { return settings_.groqApiKey; }
    void setGroqApiKey(QString v);
    QString terminalScheme() const { return settings_.terminalScheme; }
    void setTerminalScheme(QString v);
    QString fontName() const { return settings_.fontName; }
    void setFontName(QString v);
    int fontSize() const { return settings_.fontSize; }
    void setFontSize(int v);
    QString shellOverride() const { return settings_.shellOverride; }
    void setShellOverride(QString v);
    int scrollback() const { return settings_.scrollback; }
    void setScrollback(int v);
    bool restoreOnLaunch() const { return settings_.restoreOnLaunch; }
    void setRestoreOnLaunch(bool v);
    QString voiceLanguage() const { return settings_.voiceLanguage; }
    void setVoiceLanguage(QString v);
    QString voiceModel() const { return settings_.voiceModel; }
    void setVoiceModel(QString v);
    bool voiceAutoSubmit() const { return settings_.voiceAutoSubmit; }
    void setVoiceAutoSubmit(bool v);
    int appearanceMode() const { return int(settings_.appearance); }
    void setAppearanceMode(int v);
    bool autoCheckUpdates() const { return settings_.autoCheckUpdates; }
    void setAutoCheckUpdates(bool v);

    int sidebarWidth() const { return settings_.sidebarWidth; }
    void setSidebarWidth(int v);
    int sidebarMode() const { return int(settings_.sidebarMode); }
    void setSidebarMode(int v);
    bool rightSidebarVisible() const { return settings_.rightSidebarVisible; }
    void setRightSidebarVisible(bool v);

    QString activeTabId() const { return activeTabId_; }
    void setActiveTabId(QString v);

    QString filter() const { return filter_; }
    void setFilter(QString v);

signals:
    void tabsChanged();
    void activeTabIdChanged();
    void filterChanged();
    void sidebarWidthChanged();
    void sidebarModeChanged();
    void rightSidebarVisibleChanged();
    void settingsChanged();
    /// Fired when a tab's split layout changes (split added/removed/flipped).
    /// QML's Terminal/SplitContainer listen and rebuild their pane subtree.
    void tabSplitChanged(const QString& tabId);

private:
    void persistSession();
    void persistSettings();

    QVector<Tab>     tabs_;
    QString          activeTabId_;
    QString          filter_;
    AppSettings      settings_;
    std::unique_ptr<persistence::JsonStore> sessionStore_;
    std::unique_ptr<persistence::JsonStore> settingsStore_;
};

} // namespace dante
