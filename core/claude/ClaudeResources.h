// Plain-old-data structs for Claude Code resources scanned from
// `~/.claude/` and (optionally) `<cwd>/.claude/`. Mirrors the Swift
// sibling's `ClaudeResources.swift`.
//
// Header-only — no QObject, no signals. The Q_GADGET on each struct
// lets the QML engine see members as gadget properties when stored in
// a QVariant; the list models in ResourcesController project the
// same fields as roles, so QML reads `model.name` / `model.scope`
// without ever touching these structs directly.
#pragma once

#include <QHash>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>

namespace dante::claude {

enum class ResourceScope {
    User    = 0,
    Project = 1,
};

inline QString scopeLabel(ResourceScope s) {
    return s == ResourceScope::Project ? QStringLiteral("project")
                                       : QStringLiteral("user");
}

struct Skill {
    QString name;
    QString description;
    ResourceScope scope = ResourceScope::User;
    QString filePath;
    // Surfaced via tooltip / details popover — model, tools, etc.
    QHash<QString, QString> extras;
};

struct Agent {
    QString name;
    QString description;
    ResourceScope scope = ResourceScope::User;
    QString filePath;
    QString tools;
    QString model;
};

struct Mcp {
    QString name;
    ResourceScope scope = ResourceScope::User;
    // Either a stdio (command + args) or a remote (url + transport).
    QString command;
    QStringList args;
    QString url;
    QString transport;
    // File the entry was read from (~/.mcp.json or ~/.claude.json).
    QString sourcePath;

    QString summary() const {
        if (!command.isEmpty()) {
            return args.isEmpty()
                ? command
                : command + QStringLiteral(" ") + args.join(QChar(' '));
        }
        if (!url.isEmpty()) return url;
        return QStringLiteral("(no transport)");
    }
};

} // namespace dante::claude

Q_DECLARE_METATYPE(dante::claude::Skill)
Q_DECLARE_METATYPE(dante::claude::Agent)
Q_DECLARE_METATYPE(dante::claude::Mcp)
Q_DECLARE_METATYPE(QList<dante::claude::Skill>)
Q_DECLARE_METATYPE(QList<dante::claude::Agent>)
Q_DECLARE_METATYPE(QList<dante::claude::Mcp>)
