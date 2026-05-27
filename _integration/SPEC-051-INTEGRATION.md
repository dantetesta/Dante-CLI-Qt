# SPEC-051 — Git status in file tree integration

New files (already authored):
- `apps/desktop-qt/src/GitStatusProvider.h`
- `apps/desktop-qt/src/GitStatusProvider.cpp`

These are wired into `FileTreeController` (new role + member), surfaced to
QML via two existing-style accessors, and consumed by `FileTreeView.qml`
for the per-row dot + the header branch label.

---

## 1. CMakeLists.txt

### `apps/desktop-qt/CMakeLists.txt`

Add the two new sources to the `qt_add_executable(dante-cli …)` list:

```cmake
    src/FileTreeController.cpp
    src/FileTreeController.h
    src/GitStatusProvider.cpp
    src/GitStatusProvider.h
```

No new Qt component is needed — `QProcess` lives in `Qt6::Core`, which
is already linked transitively.

---

## 2. `apps/desktop-qt/src/FileTreeController.h`

Add the include + member + new role enum + property + invokable.

```cpp
#include "GitStatusProvider.h"

class FileTreeController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QFileSystemModel* model      READ model       CONSTANT)
    Q_PROPERTY(QString           rootPath   READ rootPath    WRITE setRootPath   NOTIFY rootChanged)
    Q_PROPERTY(bool              showHidden READ showHidden  WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString           gitBranch  READ gitBranch                       NOTIFY gitBranchChanged)
public:
    /// Constructor now takes the shared provider — `main.cpp` injects it.
    explicit FileTreeController(GitStatusProvider* git, QObject* parent = nullptr);

    /// Role for the per-row status letter (M / A / ?? / "").
    enum Roles { GitStatusRole = Qt::UserRole + 1000 };

    Q_INVOKABLE QString gitStatusFor(const QString& absolutePath) const;
    QString gitBranch() const { return cachedBranch_; }

signals:
    void gitBranchChanged();

private slots:
    void onWatchedDirChanged(const QString& dir);    // wire to existing watcher
    void onGitStatusUpdated(const QString& repoRoot);
    void onGitBranchUpdated(const QString& repoRoot, const QString& branch);

private:
    GitStatusProvider* git_{nullptr};
    QString            cachedBranch_;
};
```

> The `GitStatusRole` is consumed via `gitStatusFor()` from QML (simpler
> than subclassing QFileSystemModel just for one role). If you'd rather
> override `data()` directly, subclass QFileSystemModel and forward the
> `GitStatusRole` to `git_->statusForFile(filePath(idx))`.

---

## 3. `apps/desktop-qt/src/FileTreeController.cpp`

### 3a. Constructor

```cpp
FileTreeController::FileTreeController(GitStatusProvider* git, QObject* parent)
    : QObject(parent), git_(git)
{
    model_.setRootPath(QStringLiteral("/"));
    model_.setReadOnly(false);
    model_.setNameFilterDisables(false);
    applyFilter();

    if (git_) {
        connect(git_, &GitStatusProvider::statusUpdated,
                this, &FileTreeController::onGitStatusUpdated);
        connect(git_, &GitStatusProvider::branchUpdated,
                this, &FileTreeController::onGitBranchUpdated);
    }
}
```

### 3b. `setRootPath` — refresh branch

```cpp
void FileTreeController::setRootPath(const QString& path) {
    if (path == model_.rootPath()) return;
    model_.setRootPath(path);
    if (git_) {
        cachedBranch_ = git_->branchName(path);
        emit gitBranchChanged();
    }
    emit rootChanged();
}
```

### 3c. New invokable + slots

```cpp
QString FileTreeController::gitStatusFor(const QString& absolutePath) const {
    if (!git_) return {};
    return git_->statusForFile(absolutePath);
}

void FileTreeController::onWatchedDirChanged(const QString& dir) {
    if (git_) git_->invalidate(dir);
}

void FileTreeController::onGitStatusUpdated(const QString& /*repoRoot*/) {
    // Cheap: emit a generic model-data refresh. QML rebinds gitStatusFor
    // for every visible row.
    emit rootChanged();
}

void FileTreeController::onGitBranchUpdated(const QString& /*repoRoot*/,
                                            const QString& branch) {
    cachedBranch_ = branch;
    emit gitBranchChanged();
}
```

### 3d. Existing `QFileSystemWatcher`

If `FileTreeController` already has a watcher, hook its `directoryChanged`
signal into `onWatchedDirChanged`. If not, the model's `directoryLoaded`
signal works as a lightweight proxy:

```cpp
connect(&model_, &QFileSystemModel::directoryLoaded,
        this,   &FileTreeController::onWatchedDirChanged);
```

---

## 4. `apps/desktop-qt/src/main.cpp`

Include + instantiate + inject:

```cpp
#include "GitStatusProvider.h"

// … inside main, before the FileTreeController …
auto* git      = new dante::GitStatusProvider(&app);
auto* fileTree = new dante::FileTreeController(git, &app);

engine.rootContext()->setContextProperty("gitStatus", git);
```

(`gitStatus` context property is optional — only needed if other QML
files want direct access. `FileTreeView.qml` only talks to `fileTree`.)

---

## 5. `ui/qml/FileTreeView.qml`

### 5a. Header — branch label

Inside the path toolbar `RowLayout` (around line 95), after the parent-dir
button, add the branch chip:

```qml
Rectangle {
    visible: fileTree.gitBranch.length > 0
    Layout.preferredHeight: 18
    Layout.preferredWidth: branchTxt.implicitWidth + 16
    radius: 9
    color: Theme.accentDim
    border.color: Theme.accentSoft
    border.width: 1
    Text {
        id: branchTxt
        anchors.centerIn: parent
        text: "⎇ " + fileTree.gitBranch
        color: Theme.accent
        font.family: Theme.fontMono
        font.pixelSize: 10
        font.weight: Font.DemiBold
    }
}
```

### 5b. Row delegate — colored dot

Inside the row's `RowLayout` (around line 215, after the file-name Text),
append the status dot:

```qml
Rectangle {
    Layout.preferredWidth: 6
    Layout.preferredHeight: 6
    Layout.rightMargin: 6
    radius: 3
    visible: dot.length > 0
    color: dot
    property string status: fileTree.gitStatusFor(row.path())
    readonly property string dot: {
        if (status === "M" || status === "R") return Theme.warn      // orange
        if (status === "??")                  return "#3B82F6"        // blue (untracked)
        if (status === "A")                   return Theme.success    // green (staged)
        if (status === "D")                   return Theme.danger     // red
        return ""
    }
}
```

> If you'd rather render a letter badge, swap the `Rectangle` for a small
> `Text` with `text: status`; the visibility binding is the same.

---

## 6. Verification

1. Open a tab whose `rootPath` is a git repo.
2. The header chip shows the current branch (e.g. `⎇ main`).
3. Touch any file in the repo — the row gains an orange dot within
   one filesystem-watch tick.
4. Create a new file — it shows a blue dot (`??` = untracked).
5. `git add` it — dot turns green; `git commit` — dot disappears.
6. Switch to a non-repo dir (e.g. `/Volumes`) — the branch chip and
   dots disappear.
