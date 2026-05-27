#include "GitStatusProvider.h"

#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QMutexLocker>
#include <QPointer>
#include <QDebug>

namespace dante {

namespace {
    constexpr int kProcessTimeoutMs = 2000;
}

GitStatusProvider::GitStatusProvider(QObject* parent)
    : QObject(parent) {}

QString GitStatusProvider::resolveRepoRoot(const QString& dir) {
    if (dir.isEmpty()) return {};

    {
        QMutexLocker lock(&mu_);
        const auto it = dirToRoot_.constFind(dir);
        if (it != dirToRoot_.constEnd()) return it.value();
    }

    // `git rev-parse --show-toplevel` blocks for ~10 ms on a warm fs.
    // We accept the synchronous cost — the result is cached forever for
    // this dir. Non-repo dirs are memoised as "" so we don't re-fork.
    QProcess p;
    p.setProgram("git");
    p.setArguments({"rev-parse", "--show-toplevel"});
    p.setWorkingDirectory(dir);
    p.start();
    QString root;
    if (p.waitForFinished(kProcessTimeoutMs) && p.exitCode() == 0) {
        root = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    }

    QMutexLocker lock(&mu_);
    dirToRoot_.insert(dir, root);
    return root;
}

QString GitStatusProvider::branchName(const QString& dir) {
    const QString root = resolveRepoRoot(dir);
    if (root.isEmpty()) return {};

    {
        QMutexLocker lock(&mu_);
        const auto it = cache_.constFind(root);
        if (it != cache_.constEnd() && !it->branch.isEmpty()) return it->branch;
    }
    scheduleRefresh(root);
    return {};
}

QString GitStatusProvider::statusForFile(const QString& absolutePath) {
    if (absolutePath.isEmpty()) return {};

    const QFileInfo fi(absolutePath);
    const QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    const QString root = resolveRepoRoot(dir);
    if (root.isEmpty()) return {};

    QString letter;
    bool needRefresh = false;
    {
        QMutexLocker lock(&mu_);
        const auto it = cache_.constFind(root);
        if (it == cache_.constEnd()) {
            needRefresh = true;
        } else {
            letter = it->statusByPath.value(absolutePath);
        }
    }
    if (needRefresh) scheduleRefresh(root);
    return letter;
}

void GitStatusProvider::invalidate(const QString& dir) {
    const QString root = resolveRepoRoot(dir);
    if (root.isEmpty()) return;
    {
        QMutexLocker lock(&mu_);
        cache_.remove(root);
    }
    scheduleRefresh(root);
}

void GitStatusProvider::scheduleRefresh(const QString& repoRoot) {
    {
        QMutexLocker lock(&mu_);
        auto& entry = cache_[repoRoot];
        if (entry.refreshing) return;     // already in flight
        entry.refreshing = true;
        entry.repoRoot   = repoRoot;
    }

    // Async: QProcess fires `finished()` on the calling thread. We never
    // call waitForFinished here — keeps the UI thread free.
    auto* proc = new QProcess(this);
    proc->setProgram("git");
    proc->setArguments({"status", "--porcelain=v1", "--branch"});
    proc->setWorkingDirectory(repoRoot);

    QPointer<GitStatusProvider> self(this);
    connect(proc, &QProcess::errorOccurred, this, [self, proc, repoRoot](QProcess::ProcessError) {
        if (!self) { proc->deleteLater(); return; }
        QMutexLocker lock(&self->mu_);
        auto it = self->cache_.find(repoRoot);
        if (it != self->cache_.end()) it->refreshing = false;
        proc->deleteLater();
    });
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [self, proc, repoRoot](int exitCode, QProcess::ExitStatus) {
        if (!self) { proc->deleteLater(); return; }
        if (exitCode != 0) {
            QMutexLocker lock(&self->mu_);
            auto it = self->cache_.find(repoRoot);
            if (it != self->cache_.end()) it->refreshing = false;
            proc->deleteLater();
            return;
        }
        const QByteArray out = proc->readAllStandardOutput();
        proc->deleteLater();

        QHash<QString, QString> byPath;
        QString branch;
        const QList<QByteArray> lines = out.split('\n');
        for (const auto& raw : lines) {
            const QString line = QString::fromUtf8(raw);
            if (line.isEmpty()) continue;
            if (line.startsWith("## ")) {
                // "## main...origin/main" or "## HEAD (no branch)"
                QString rest = line.mid(3);
                const int sep = rest.indexOf("...");
                if (sep > 0) rest = rest.left(sep);
                branch = rest.trimmed();
                continue;
            }
            const auto pair = parsePorcelainLine(line, repoRoot);
            if (!pair.first.isEmpty()) byPath.insert(pair.first, pair.second);
        }

        {
            QMutexLocker lock(&self->mu_);
            auto& entry = self->cache_[repoRoot];
            entry.repoRoot     = repoRoot;
            entry.branch       = branch;
            entry.statusByPath = byPath;
            entry.fetchedAt    = QDateTime::currentDateTime();
            entry.refreshing   = false;
        }
        emit self->branchUpdated(repoRoot, branch);
        emit self->statusUpdated(repoRoot);
    });

    proc->start();
}

QPair<QString, QString>
GitStatusProvider::parsePorcelainLine(const QString& line, const QString& repoRoot) {
    // Porcelain v1: "XY path" where XY is the 2-char index/worktree state.
    // Renames look like: "R  oldname -> newname".
    if (line.size() < 4) return {{}, {}};
    const QString xy = line.left(2);
    QString rest = line.mid(3);   // skip "XY "

    // For renames we want the destination path.
    const int arrow = rest.indexOf(" -> ");
    if (arrow >= 0) rest = rest.mid(arrow + 4);

    // Strip optional quotes (porcelain quotes paths with whitespace).
    if (rest.startsWith('"') && rest.endsWith('"') && rest.size() >= 2)
        rest = rest.mid(1, rest.size() - 2);

    const QString abs = QDir(repoRoot).absoluteFilePath(rest);

    QString letter;
    if (xy == "??")                                 letter = "??";
    else if (xy.contains('A'))                      letter = "A";
    else if (xy.contains('D'))                      letter = "D";
    else if (xy.contains('R'))                      letter = "R";
    else if (xy.contains('M'))                      letter = "M";
    else                                            letter = xy.trimmed();

    return { abs, letter };
}

} // namespace dante
