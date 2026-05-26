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

    /* ─── Settings ─── */
    QString groqApiKey() const { return settings_.groqApiKey; }
    void setGroqApiKey(QString v);
    QString terminalScheme() const { return settings_.terminalScheme; }
    void setTerminalScheme(QString v);
    QString fontName() const { return settings_.fontName; }
    void setFontName(QString v);
    int fontSize() const { return settings_.fontSize; }
    void setFontSize(int v);

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
