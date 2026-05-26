// Light-weight JSON file IO with atomic-rename writes and debounced saves.
// Mirrors `PersistenceRoot.swift` semantics from the Swift sibling.
#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QTimer>
#include <QObject>
#include <functional>

namespace dante::persistence {

class JsonStore : public QObject {
    Q_OBJECT
public:
    /// `filename` is relative to the AppData dir (e.g. "favorites.json").
    /// `debounceMs` controls write coalescing (0 = synchronous on every set).
    explicit JsonStore(QString filename, int debounceMs = 300, QObject* parent = nullptr);

    /// Read once at startup; returns the file as a QJsonDocument.
    /// If the file is missing or invalid, returns the supplied fallback.
    QJsonDocument read(const QJsonDocument& fallback = {}) const;

    /// Schedule a save. Subsequent calls within `debounceMs` coalesce.
    void scheduleWrite(QJsonDocument doc);

    /// Flush any pending write synchronously (call before app shutdown).
    void flush();

signals:
    void writeFailed(const QString& reason);

private:
    void doWrite();

    QString    filename_;
    QTimer     timer_;
    QJsonDocument pending_;
    bool       hasPending_{false};
};

} // namespace dante::persistence
