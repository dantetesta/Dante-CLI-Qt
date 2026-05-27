#include "GeneratorsModel.h"

#include <QHash>
#include <QVariantMap>

namespace dante {

GeneratorsModel::GeneratorsModel(QObject* parent)
    : QAbstractListModel(parent) {}

int GeneratorsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return registry_.all().size();
}

QVariant GeneratorsModel::data(const QModelIndex& index, int role) const {
    const auto items = registry_.all();
    if (!index.isValid() || index.row() < 0 || index.row() >= items.size())
        return {};
    const auto& g = items.at(index.row());
    switch (role) {
        case IdRole:          return g.id;
        case CategoryRole:    return g.category;
        case NameRole:        return g.name;
        case DescriptionRole: return g.description;
        case TemplateRole:    return g.templateText;
        case IconRole:        return g.icon;
        default:              return {};
    }
}

QHash<int, QByteArray> GeneratorsModel::roleNames() const {
    return {
        {IdRole,          "id"},
        {CategoryRole,    "category"},
        {NameRole,        "name"},
        {DescriptionRole, "description"},
        {TemplateRole,    "templateText"},
        {IconRole,        "icon"},
    };
}

QStringList GeneratorsModel::categories() const {
    return registry_.categories();
}

QString GeneratorsModel::pick(const QString& id) const {
    return registry_.findById(id).templateText;
}

QVariantList GeneratorsModel::filtered(const QString& category,
                                       const QString& query) const {
    QVariantList out;
    const QString needle = query.trimmed().toLower();
    for (const auto& g : registry_.all()) {
        if (!category.isEmpty()
            && g.category.compare(category, Qt::CaseInsensitive) != 0) continue;
        if (!needle.isEmpty()) {
            const bool hit = g.name.toLower().contains(needle)
                          || g.description.toLower().contains(needle)
                          || g.id.toLower().contains(needle)
                          || g.category.toLower().contains(needle);
            if (!hit) continue;
        }
        QVariantMap m;
        m.insert(QStringLiteral("id"),           g.id);
        m.insert(QStringLiteral("category"),     g.category);
        m.insert(QStringLiteral("name"),         g.name);
        m.insert(QStringLiteral("description"),  g.description);
        m.insert(QStringLiteral("templateText"), g.templateText);
        m.insert(QStringLiteral("icon"),         g.icon);
        out.append(m);
    }
    return out;
}

} // namespace dante
