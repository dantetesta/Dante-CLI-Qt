// Resolve the platform's AppData directory + helpers for our JSON files.
// On Windows: %APPDATA%\Dante CLI\
// On macOS:   ~/Library/Application Support/Dante CLI/
#pragma once
#include <QDir>
#include <QString>

namespace dante::persistence {
    /// Returns the per-user data dir, creating it if necessary. Empty on failure.
    QDir appDataDir();
    /// Convenience: appDataDir().filePath(name).
    QString file(QString name);
}
