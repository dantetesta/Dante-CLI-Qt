// Domain models — mirror of the Swift sibling's `Models/*.swift`.
// Plain C++ structs with Qt metatype registration so they cross the QML
// boundary without copying logic into QML.

#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMetaType>
#include <QHash>
#include <optional>

namespace dante {

// -----------------------------------------------------------------------------
// Tab system
// -----------------------------------------------------------------------------

enum class TabKind { Terminal, Editor, Preview, Video, Browser };

/// Mirrors `TerminalTab.swift`.
struct Tab {
    QString id;
    QString title{"Terminal"};
    QString color{"#0A84FF"};        // "#hex" or "grad:name"
    QString emoji{"💻"};
    bool    pinned{false};
    TabKind kind{TabKind::Terminal};

    // Live cwd from OSC 7. Empty until shell emits one.
    QString cwd;

    // Per-tab overrides (null/empty = inherit from AppSettings).
    QString customScheme;
    QString customFontName;
    int     customFontSize{0};

    QString sessionId; // PTY session id (terminal only).

    // Split-pane state (minimal 2-pane model, mirrors Swift sibling's
    // common case — recursive nesting deliberately out of scope).
    //   splitMode == ""           → single pane (only `sessionId`).
    //   splitMode == "vertical"   → two panes side-by-side.
    //   splitMode == "horizontal" → two panes stacked.
    // Second pane uses `secondSessionId` (UUID-stable as `id + ":b"`).
    QString splitMode;
    QString secondSessionId;
    double  splitFraction{0.5};   // 0..1 — first pane's share (left if vertical, top if horizontal)
};

// -----------------------------------------------------------------------------
// Sidebar collections
// -----------------------------------------------------------------------------

struct Favorite {
    QString    id;
    QString    name;
    QString    path;
    QStringList tags;
    QString    colorHex;
    QString    emoji;
    QString    initialCommand;
    QDateTime  createdAt;
};

struct Snippet {
    QString    id;
    QString    name;
    QString    command;
    QStringList tags;
    QString    emoji;
    QDateTime  createdAt;
};

enum class CredentialKind { Ssh, Ftp, Api, Database, Custom };

struct CredentialField {
    QString label;
    QString value;
    bool    masked{false};
};

struct Credential {
    QString id;
    QString name;
    CredentialKind kind{CredentialKind::Ssh};
    QVector<CredentialField> fields;
    QString notes;
    QString emoji;
    QStringList tags;
    QDateTime createdAt;
};

// -----------------------------------------------------------------------------
// AI providers
// -----------------------------------------------------------------------------

struct AIProvider {
    QString id;
    QString name;
    QString command;
    QStringList args;
    QString sfSymbol;
    QString colorHex{"#7C82FF"};
    QString emoji;
    bool    enabled{true};
};

// -----------------------------------------------------------------------------
// App settings
// -----------------------------------------------------------------------------

enum class AppearanceMode { System, Dark, Light };
enum class SidebarMode    { Favorites, Files, Snippets, Credentials };

struct AppSettings {
    AppearanceMode appearance{AppearanceMode::Dark};
    QString fontName{"JetBrains Mono"};
    int     fontSize{13};
    QString terminalScheme{"TokyoNight"};
    QString groqApiKey;
    bool    autoCheckUpdates{true};
    QString updateManifestURL{"https://dantetesta.com.br/dante-cli/manifest.json"};
    QString voiceLanguage{"pt"};
    QString voiceModel{"whisper-large-v3-turbo"};
    bool    voiceAutoSubmit{false};
    SidebarMode sidebarMode{SidebarMode::Favorites};
    bool    rightSidebarVisible{false};
    int     sidebarWidth{280};
    QString fileBrowserRoot;
    bool    fileBrowserShowsHidden{false};
    QString shellOverride;
    int     scrollback{50000};
    bool    restoreOnLaunch{true};
    QStringList recentEmojis{"💻","🚀","🐳","🦀","🐍","📦","⚡","🔥","✨","🎯"};
};

} // namespace dante

Q_DECLARE_METATYPE(dante::Tab)
Q_DECLARE_METATYPE(dante::Favorite)
Q_DECLARE_METATYPE(dante::Snippet)
Q_DECLARE_METATYPE(dante::Credential)
Q_DECLARE_METATYPE(dante::AIProvider)
