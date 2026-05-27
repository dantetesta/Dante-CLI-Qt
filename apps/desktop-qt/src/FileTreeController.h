// File-system tree controller — wraps QFileSystemModel for the Pastas tab.
// Q_INVOKABLE helpers cover the operations the sidebar's right-click menu
// fires: open externally, reveal in Finder/Explorer, rename, delete, copy,
// new folder, plus paths-by-index lookups used for drag-and-drop and for
// the "cd no terminal" / "abrir no slot" shortcuts.
//
// We do NOT subclass — QFileSystemModel is exposed directly via a
// `Q_PROPERTY(QFileSystemModel* model)` so QML's TreeView can bind to it.
#pragma once

#include <QFileSystemModel>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringList>

namespace dante {

class GitStatusProvider;

class FileTreeController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QFileSystemModel* model      READ model       CONSTANT)
    Q_PROPERTY(QString           rootPath   READ rootPath    WRITE setRootPath   NOTIFY rootChanged)
    Q_PROPERTY(bool              showHidden READ showHidden  WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString           gitBranch  READ gitBranch                       NOTIFY gitBranchChanged)
public:
    explicit FileTreeController(QObject* parent = nullptr);

    /// SPEC-051 — inject a shared provider. Called from main.cpp after both
    /// objects exist so the constructor signature stays additive.
    void setGitProvider(GitStatusProvider* git);
    QString gitBranch() const { return cachedBranch_; }
    Q_INVOKABLE QString gitStatusFor(const QString& absolutePath) const;

    QFileSystemModel* model() const { return const_cast<QFileSystemModel*>(&model_); }

    QString rootPath() const;
    void setRootPath(const QString& path);

    /// Root QModelIndex for binding TreeView::rootIndex.
    Q_INVOKABLE QModelIndex rootIndex() const;

    /// Show / hide dotfiles and Hidden-attributed files.
    bool showHidden() const { return showHidden_; }
    void setShowHidden(bool v);

    /// Quick-jump destinations for the header (Home, /, /Volumes, drives).
    /// Each entry is { "label": str, "path": str, "icon": str }.
    Q_INVOKABLE QVariantList quickPlaces() const;

    /// Resolve a model index to absolute filesystem path.
    Q_INVOKABLE QString filePath(const QModelIndex& idx) const;
    Q_INVOKABLE QString fileName(const QModelIndex& idx) const;
    Q_INVOKABLE bool    isDir(const QModelIndex& idx) const;

    /// Open the file/folder with the OS default handler.
    Q_INVOKABLE void openExternally(const QString& path) const;
    /// Reveal the file/folder in Finder (mac) / Explorer (win).
    Q_INVOKABLE void revealInOsExplorer(const QString& path) const;
    /// Copy the absolute path to the system clipboard.
    Q_INVOKABLE void copyPath(const QString& path) const;

    /// Mutate-the-filesystem operations. Each one is best-effort and emits
    /// `operationFailed(reason)` on any IO error so the QML can toast it.
    Q_INVOKABLE bool createFolder(const QString& parentPath, const QString& name);
    Q_INVOKABLE bool createFile(const QString& parentPath, const QString& name);
    Q_INVOKABLE bool renamePath(const QString& path, const QString& newName);
    Q_INVOKABLE bool deletePath(const QString& path);     // moves to trash
    Q_INVOKABLE bool duplicatePath(const QString& path);  // creates "name copy.ext"

signals:
    void rootChanged();
    void showHiddenChanged();
    void operationFailed(const QString& reason);
    void gitBranchChanged();

private:
    void applyFilter();

    QFileSystemModel    model_;
    bool                showHidden_{false};
    GitStatusProvider*  git_{nullptr};
    QString             cachedBranch_;
};

} // namespace dante
