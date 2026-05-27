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
    Q_PROPERTY(QStringList recentEmojis    READ recentEmojis        NOTIFY settingsChanged)
    Q_PROPERTY(QString uiLanguage          READ uiLanguage          WRITE setUiLanguage          NOTIFY settingsChanged)
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

    /* ─── Tab-chip mutations (SPEC-120) ─── */
    /// Rename. Empty title falls back to "Terminal" to avoid invisible chips.
    Q_INVOKABLE void setTabTitle(const QString& tabId, const QString& title);
    Q_INVOKABLE void setTabColor(const QString& tabId, const QString& colorHex);
    Q_INVOKABLE void setTabEmoji(const QString& tabId, const QString& emoji);
    /// Per-tab scheme override (id from ThemeRegistry; empty == inherit settings).
    Q_INVOKABLE void setTabScheme(const QString& tabId, const QString& schemeId);
    /// Pinned tabs are skipped by closeTab() (matches Swift sibling).
    Q_INVOKABLE void setTabPinned(const QString& tabId, bool pinned);
    Q_INVOKABLE bool isTabPinned(const QString& tabId) const;
    /// Clone all visible metadata except sessionId (new shell session).
    Q_INVOKABLE QString duplicateTab(const QString& tabId);

    /* ─── SPEC-021 — Editor tab kind ─── */
    /// Create (or focus, if already open) an editor tab for `path`. Returns
    /// the tab id. Reads the file synchronously up to ~5 MB; bigger files
    /// surface an error via toast (TODO) and a tab is not created.
    Q_INVOKABLE QString openFileInEditor(const QString& path);
    /// Persist the editor buffer to disk. Clears the dirty flag on success.
    /// Emits operationFailed-style log lines on IO error (TODO toast).
    Q_INVOKABLE bool saveEditor(const QString& tabId);
    /// Update the in-memory buffer (called from EditorView on every key).
    Q_INVOKABLE void setEditorContent(const QString& tabId, const QString& content);
    /// Read-only getters (QML uses them through bindings keyed on `_bump`).
    Q_INVOKABLE QString editorPath(const QString& tabId) const;
    Q_INVOKABLE QString editorContent(const QString& tabId) const;
    Q_INVOKABLE QString editorLanguage(const QString& tabId) const;
    Q_INVOKABLE bool    editorDirty(const QString& tabId) const;
    Q_INVOKABLE int     tabKind(const QString& tabId) const; // 0=Terminal, 1=Editor, …
    Q_INVOKABLE QString tabKindString(const QString& tabId) const;
    Q_INVOKABLE QString openCalculatorTab();

    /* ─── SPEC-023 — Video tab kind ─── */
    Q_INVOKABLE QString newVideoTab(const QString& title, const QString& path);
    Q_INVOKABLE QString tabVideoPath(const QString& tabId) const;
    Q_INVOKABLE void    setTabVideoPath(const QString& tabId, const QString& path);

    /* ─── SPEC-022 — Browser tab kind ─── */
    Q_INVOKABLE QString newBrowserTab(const QString& title, const QString& url);
    Q_INVOKABLE QString tabBrowserUrl(const QString& tabId) const;
    Q_INVOKABLE void    setTabBrowserUrl(const QString& tabId, const QString& url);

    /* ─── SPEC-170 — Drag tab → grid slot ─── */
    Q_INVOKABLE void    attachTabToSlot(const QString& sourceTabId,
                                         const QString& hostTabId,
                                         int cellIndex);
    Q_INVOKABLE QString tabAtSlot(const QString& hostTabId, int cellIndex) const;
    Q_INVOKABLE void    detachTabFromSlot(const QString& hostTabId, int cellIndex);
    /// Convenience accessors used by the placeholder card in GridWorkspace.
    Q_INVOKABLE QString tabTitle(const QString& tabId) const;
    Q_INVOKABLE QString tabEmoji(const QString& tabId) const;

    /* ─── SPEC-160 — i18n language preference ─── */
    QString uiLanguage() const;
    void    setUiLanguage(const QString& locale);

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

    /* ─── Multi-pane GRID workspace (cols × rows + merge spans) ─── */
    /// Returns 0 when the tab is in single-pane or 2-pane split mode.
    Q_INVOKABLE int tabGridCols(const QString& tabId) const;
    Q_INVOKABLE int tabGridRows(const QString& tabId) const;
    /// Spans map as cellIndex (string) → {"cols": N, "rows": M}.
    Q_INVOKABLE QVariantMap tabGridSpans(const QString& tabId) const;
    /// Switch the tab into grid-workspace mode. Pass cols=0 OR rows=0 to
    /// clear back to single-pane (any active b-PTY should be killed by the
    /// caller before invoking — same convention as splitActive("")).
    Q_INVOKABLE void setTabGrid(const QString& tabId, int cols, int rows, const QVariantMap& spans);

    /* ─── SPEC-110 — recursive pane tree (N panes) ─── */
    /// Returns the pane tree for a tab (empty map when single-pane).
    Q_INVOKABLE QVariantMap tabPaneTree(const QString& tabId) const;
    /// Splits the leaf whose sessionId matches `targetSessionId`. If the tab
    /// has no tree yet, it's promoted to a 2-pane tree first. Returns the
    /// newly created leaf's sessionId.
    Q_INVOKABLE QString splitPane(const QString& tabId,
                                  const QString& targetSessionId,
                                  const QString& axis /* "vertical"|"horizontal" */);
    /// Removes the leaf identified by `targetSessionId`. Collapses the
    /// sibling up. If the tree degenerates to a single leaf it's stored as
    /// Tab.sessionId and paneTree is cleared.
    Q_INVOKABLE void closePaneInTree(const QString& tabId,
                                     const QString& targetSessionId);
    /// Persist a new ratio for the split node at `path` (array of 0/1
    /// indices descending the tree). 0 = left/top child, 1 = right/bottom.
    Q_INVOKABLE void setPaneRatio(const QString& tabId,
                                  const QVariantList& path,
                                  double ratio);

    /* ─── Settings ─── */
    QStringList recentEmojis() const { return settings_.recentEmojis; }
    /// Move `emoji` to the head of the recents list. Dedups + caps to 32.
    /// Empty strings are ignored. Persisted via settings.json.
    Q_INVOKABLE void pushRecentEmoji(const QString& emoji);

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
