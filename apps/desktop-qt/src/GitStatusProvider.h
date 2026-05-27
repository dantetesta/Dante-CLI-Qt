// SPEC-051 — Git status cache for the file tree.
//
// Async (QProcess) wrapper that runs `git status --porcelain=v1` per
// directory and caches the result. FileTreeController calls
// `statusForFile(absolutePath)` on data(GitStatusRole) and
// `invalidate(dir)` whenever its QFileSystemWatcher fires.
//
// Lookup is by *enclosing repo root* — we shell out `git rev-parse
// --show-toplevel` lazily and cache that too. Files outside any repo
// resolve to an empty status, never null.
#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMutex>

class QProcess;

namespace dante {

class GitStatusProvider : public QObject {
    Q_OBJECT
public:
    explicit GitStatusProvider(QObject* parent = nullptr);

    /// Active branch for the repo containing `dir`. Empty when `dir` is
    /// not under a git work-tree.
    Q_INVOKABLE QString branchName(const QString& dir);

    /// Status letter for a single file:
    ///   "M"   modified (worktree or index)
    ///   "A"   added (staged)
    ///   "D"   deleted
    ///   "R"   renamed
    ///   "??"  untracked
    ///   ""    clean / not in a repo
    Q_INVOKABLE QString statusForFile(const QString& absolutePath);

    /// Force a re-fetch on the next `statusForFile` / `branchName`
    /// call for the repo containing `dir`. Cheap — just clears the
    /// cache entry; the actual `git` re-runs lazily.
    Q_INVOKABLE void invalidate(const QString& dir);

signals:
    /// Fired after a background refresh completes for the given repo
    /// root. FileTreeController listens and pings the model so the QML
    /// rows repaint with the new dots.
    void statusUpdated(const QString& repoRoot);
    void branchUpdated(const QString& repoRoot, const QString& branch);

private:
    struct CacheEntry {
        QString    repoRoot;
        QString    branch;
        QHash<QString, QString> statusByPath;   // absolute path → letter
        QDateTime  fetchedAt;
        bool       refreshing{false};
    };

    /// Resolve the repo root for `dir` (cached). Returns empty when
    /// outside any repo.
    QString resolveRepoRoot(const QString& dir);

    /// Kick off (or no-op if already in flight) an async refresh for
    /// `repoRoot`. Result is dropped straight into `cache_`.
    void scheduleRefresh(const QString& repoRoot);

    /// Parse one porcelain v1 line into (absolutePath, statusLetter).
    /// `repoRoot` is needed because porcelain emits repo-relative paths.
    static QPair<QString, QString> parsePorcelainLine(const QString& line,
                                                     const QString& repoRoot);

    QMutex                        mu_;             // guards cache_ + roots_
    QHash<QString, CacheEntry>    cache_;          // repoRoot → entry
    QHash<QString, QString>       dirToRoot_;      // any dir → repoRoot ("" means non-repo)
};

} // namespace dante
