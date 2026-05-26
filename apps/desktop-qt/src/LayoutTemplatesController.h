// Persists named split layouts so the user can reuse them. JSON store lives
// at %APPDATA%/Dante CLI/layout-templates.json (mac: ~/Library/Application
// Support/...). Each template captures cols/rows + a sparse map of merged
// cell spans. The QML LayoutDesigner reads `list`, lets the user click a
// card to reload state, and writes back through `addOrUpdate`.
//
// Mirrors the Swift LayoutTemplate + LayoutTemplatesStore pair, simplified:
// no colWidths/rowHeights persistence yet (the current SplitContainer doesn't
// support custom seam ratios on multi-pane grids).
#pragma once

#include "persistence/JsonStore.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace dante {

class LayoutTemplatesController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList list READ list NOTIFY listChanged)
public:
    explicit LayoutTemplatesController(QObject* parent = nullptr);

    QVariantList list() const { return list_; }

    /// Persist a new template (or overwrite if `id` non-empty and matches).
    /// `spans` is a JS object of cellIndex → {cols, rows}.
    /// Returns the saved template's id.
    Q_INVOKABLE QString save(const QString& id,
                             const QString& name,
                             const QString& emoji,
                             int cols, int rows,
                             const QVariantMap& spans);

    /// Drop a template by id. No-op if not found.
    Q_INVOKABLE void remove(const QString& id);

    /// Fetch a single template as a QVariantMap (keys: id, name, emoji,
    /// cols, rows, spans, createdAt). Returns an empty map when unknown.
    Q_INVOKABLE QVariantMap get(const QString& id) const;

    /// Count of visible panels for a given spans map — for the card subtitle.
    Q_INVOKABLE int visiblePanelCount(int cols, int rows, const QVariantMap& spans) const;

signals:
    void listChanged();

private:
    void hydrate();
    void persist();

    std::unique_ptr<persistence::JsonStore> store_;
    QVariantList list_;     // each entry is a QVariantMap with the keys above
};

} // namespace dante
