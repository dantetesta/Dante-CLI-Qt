// Async filesystem scanner for Claude Code resources.
//
// Walks `~/.claude/skills/<name>/SKILL.md`, `~/.claude/agents/*.md`,
// `~/.mcp.json` and the project-local equivalents under the supplied
// cwd. Each rescan happens off-thread (QtConcurrent::run) and emits
// type-specific `loaded` signals on the UI thread when done.
//
// QFileSystemWatcher coalesces filesystem events into a single debounced
// rescan (300ms) so a save burst from a text editor doesn't trigger ten
// scans in a row.
#pragma once

#include "ClaudeResources.h"

#include <QFileSystemWatcher>
#include <QFuture>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace dante::claude {

class ClaudeResourceScanner : public QObject {
    Q_OBJECT
public:
    explicit ClaudeResourceScanner(QObject* parent = nullptr);
    ~ClaudeResourceScanner() override;

    // Update the project-scope cwd. Empty disables project scanning.
    // Re-arms the watcher and triggers a rescan.
    void setProjectCwd(const QString& cwd);
    QString projectCwd() const { return projectCwd_; }

    // Trigger an async rescan. Safe to call repeatedly — concurrent
    // calls coalesce via the 300ms debounce.
    void rescan();

public slots:
    void rescanImmediate();

signals:
    void skillsLoaded(const QList<dante::claude::Skill>& skills);
    void agentsLoaded(const QList<dante::claude::Agent>& agents);
    void mcpsLoaded(const QList<dante::claude::Mcp>& mcps);
    void scanningChanged(bool scanning);

private:
    // Static helpers — pure file I/O, safe to call from the worker.
    static QList<Skill> scanSkillsDir(const QString& path, ResourceScope scope);
    static QList<Agent> scanAgentsDir(const QString& path, ResourceScope scope);
    static QList<Mcp> scanMcpsFile(const QString& path, ResourceScope scope);

    void rearmWatcher();

    QFileSystemWatcher watcher_;
    QTimer debounce_;
    QString projectCwd_;
    bool scanning_ = false;
};

// Minimal YAML frontmatter parser. Handles `key: value` (with optional
// quotes) and `key: |` / `key: >` block scalars. No nested mappings, no
// lists. Returns an empty hash when the file has no frontmatter.
class YamlFrontmatter {
public:
    static QHash<QString, QString> parse(const QString& content);
};

} // namespace dante::claude
