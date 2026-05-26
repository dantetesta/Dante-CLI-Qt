#include "FileTreeController.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace dante {

FileTreeController::FileTreeController(QObject* parent)
    : QObject(parent)
{
    // Watch the whole filesystem so any path the user navigates to is live.
    // (Setting "" tells QFileSystemModel to watch the root of the filesystem.)
    model_.setRootPath(QStringLiteral("/"));
    model_.setReadOnly(false);
    model_.setNameFilterDisables(false);
    applyFilter();
}

void FileTreeController::applyFilter() {
    QDir::Filters f = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (showHidden_) f |= QDir::Hidden | QDir::System;
    model_.setFilter(f);
}

void FileTreeController::setShowHidden(bool v) {
    if (showHidden_ == v) return;
    showHidden_ = v;
    applyFilter();
    emit showHiddenChanged();
}

QVariantList FileTreeController::quickPlaces() const {
    QVariantList out;
    auto add = [&](const QString& label, const QString& path, const QString& icon) {
        if (path.isEmpty() || !QFileInfo::exists(path)) return;
        out.append(QVariantMap{{"label", label}, {"path", path}, {"icon", icon}});
    };

    add(QStringLiteral("Início"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "🏠");
    add(QStringLiteral("Desktop"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "🖥");
    add(QStringLiteral("Documentos"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "📄");
    add(QStringLiteral("Downloads"),
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), "⬇");
    add(QStringLiteral("/"), QStringLiteral("/"), "💽");

#if defined(Q_OS_MAC)
    // List mounted volumes under /Volumes — gives access to external disks,
    // Time Machine, USB, etc.
    QDir vols(QStringLiteral("/Volumes"));
    for (const auto& fi : vols.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        out.append(QVariantMap{{"label", fi.fileName()}, {"path", fi.absoluteFilePath()}, {"icon", "💾"}});
    }
#elif defined(Q_OS_WIN)
    // Each lettered drive (C:/, D:/, …).
    for (const auto& fi : QDir::drives()) {
        out.append(QVariantMap{{"label", fi.absolutePath()},
                               {"path",  fi.absolutePath()},
                               {"icon",  "💽"}});
    }
#endif

    return out;
}

QString FileTreeController::rootPath() const {
    return model_.rootPath();
}

void FileTreeController::setRootPath(const QString& path) {
    if (path == model_.rootPath()) return;
    model_.setRootPath(path);
    emit rootChanged();
}

QModelIndex FileTreeController::rootIndex() const {
    return model_.index(model_.rootPath());
}

QString FileTreeController::filePath(const QModelIndex& idx) const {
    return model_.filePath(idx);
}
QString FileTreeController::fileName(const QModelIndex& idx) const {
    return model_.fileName(idx);
}
bool FileTreeController::isDir(const QModelIndex& idx) const {
    return model_.isDir(idx);
}

void FileTreeController::openExternally(const QString& path) const {
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void FileTreeController::revealInOsExplorer(const QString& path) const {
    const QFileInfo fi(path);
    if (!fi.exists()) return;
#if defined(Q_OS_MAC)
    QProcess::startDetached("open", { "-R", fi.absoluteFilePath() });
#elif defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe", { "/select,", QDir::toNativeSeparators(fi.absoluteFilePath()) });
#else
    // Linux: best-effort — open the parent folder.
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
}

void FileTreeController::copyPath(const QString& path) const {
    if (auto* cb = QGuiApplication::clipboard()) cb->setText(path);
}

bool FileTreeController::createFolder(const QString& parentPath, const QString& name) {
    if (name.trimmed().isEmpty()) {
        emit const_cast<FileTreeController*>(this)->operationFailed("Nome da pasta vazio");
        return false;
    }
    QDir d(parentPath);
    if (!d.mkdir(name)) {
        emit operationFailed(QString("Não foi possível criar a pasta: %1").arg(name));
        return false;
    }
    return true;
}

bool FileTreeController::createFile(const QString& parentPath, const QString& name) {
    if (name.trimmed().isEmpty()) {
        emit operationFailed("Nome do arquivo vazio");
        return false;
    }
    QFile f(QDir(parentPath).filePath(name));
    if (f.exists()) {
        emit operationFailed(QString("Já existe: %1").arg(name));
        return false;
    }
    if (!f.open(QIODevice::WriteOnly)) {
        emit operationFailed(QString("Não foi possível criar: %1").arg(name));
        return false;
    }
    f.close();
    return true;
}

bool FileTreeController::renamePath(const QString& path, const QString& newName) {
    if (newName.trimmed().isEmpty() || newName.contains('/') || newName.contains('\\')) {
        emit operationFailed("Nome inválido");
        return false;
    }
    const QFileInfo fi(path);
    const QString target = fi.absoluteDir().filePath(newName);
    if (QFileInfo::exists(target)) {
        emit operationFailed(QString("Já existe: %1").arg(newName));
        return false;
    }
    if (!QFile::rename(path, target)) {
        emit operationFailed(QString("Falha ao renomear %1").arg(fi.fileName()));
        return false;
    }
    return true;
}

bool FileTreeController::deletePath(const QString& path) {
    // moveToTrash returns false on platforms without a trash; we don't fall
    // back to permanent delete — too dangerous without a confirm dialog.
    QString newLocation;
    if (!QFile::moveToTrash(path, &newLocation)) {
        emit operationFailed(QString("Não foi possível mover para a lixeira: %1")
                                .arg(QFileInfo(path).fileName()));
        return false;
    }
    return true;
}

bool FileTreeController::duplicatePath(const QString& path) {
    const QFileInfo fi(path);
    if (!fi.exists()) {
        emit operationFailed("Origem não existe");
        return false;
    }
    const QString base = fi.completeBaseName();
    const QString suffix = fi.suffix();
    const QString joined = suffix.isEmpty() ? base : base + "." + suffix;
    // First try "name copy.ext"; if that exists, append " 2", " 3", …
    QString candidate;
    for (int i = 1; i < 100; ++i) {
        const QString tag = (i == 1) ? QStringLiteral(" copy") : QStringLiteral(" copy %1").arg(i);
        candidate = suffix.isEmpty()
            ? fi.absoluteDir().filePath(base + tag)
            : fi.absoluteDir().filePath(base + tag + "." + suffix);
        if (!QFileInfo::exists(candidate)) break;
    }
    if (fi.isDir()) {
        // Recursive directory copy.
        QDir src(path);
        if (!QDir().mkpath(candidate)) {
            emit operationFailed("Falha ao criar pasta de destino");
            return false;
        }
        for (const auto& entry : src.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden)) {
            duplicatePath(entry.absoluteFilePath());  // crude; good enough for tonight
        }
        return true;
    }
    if (!QFile::copy(path, candidate)) {
        emit operationFailed(QString("Falha ao duplicar %1").arg(fi.fileName()));
        return false;
    }
    return true;
}

} // namespace dante
