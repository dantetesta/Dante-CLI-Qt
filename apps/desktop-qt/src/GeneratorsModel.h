// SPEC-081 — QAbstractListModel wrapper around GeneratorsRegistry.
//
// Exposed to QML as `generators`. Reads the registry once at construction
// and serves the catalog through standard model roles so a ListView/Repeater
// can render cards directly. `pick(id)` returns the template text, which the
// QML side then hands off to AutoFillController for placeholder handling.
#pragma once

#include "generators/GeneratorsRegistry.h"

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace dante {

class GeneratorsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QStringList categories READ categories CONSTANT)
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        CategoryRole,
        NameRole,
        DescriptionRole,
        TemplateRole,
        IconRole
    };

    explicit GeneratorsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList categories() const;

    // Returns the templateText for the given id; empty string when missing.
    // QML callers hand the result to `autoFill.prepare(text)`.
    Q_INVOKABLE QString pick(const QString& id) const;

    // Returns a list of QVariantMap (one per generator) filtered by category
    // and/or a free-text query. Pass an empty string to disable a filter.
    // Useful for the palette UI which needs both filters simultaneously.
    Q_INVOKABLE QVariantList filtered(const QString& category,
                                      const QString& query) const;

private:
    generators::GeneratorsRegistry registry_;
};

} // namespace dante
