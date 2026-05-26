#include "FavoritesModel.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

namespace dante {

FavoritesModel::FavoritesModel(QObject* parent)
    : QAbstractListModel(parent)
    , store_(std::make_unique<persistence::JsonStore>("favorites.json", 300, this))
{}

void FavoritesModel::hydrate() {
    beginResetModel();
    items_.clear();
    const auto arr = store_->read({}).array();
    for (const auto& v : arr) {
        const auto o = v.toObject();
        Favorite f;
        f.id    = o.value("id").toString();
        f.name  = o.value("name").toString();
        f.path  = o.value("path").toString();
        f.emoji = o.value("emoji").toString();
        f.colorHex = o.value("colorHex").toString();
        const auto tags = o.value("tags").toArray();
        for (const auto& t : tags) f.tags.append(t.toString());
        items_.append(f);
    }
    endResetModel();
}

int FavoritesModel::rowCount(const QModelIndex& p) const {
    return p.isValid() ? 0 : items_.size();
}

QVariant FavoritesModel::data(const QModelIndex& idx, int role) const {
    if (idx.row() < 0 || idx.row() >= items_.size()) return {};
    const auto& f = items_.at(idx.row());
    switch (role) {
        case IdRole:    return f.id;
        case NameRole:  return f.name;
        case PathRole:  return f.path;
        case EmojiRole: return f.emoji;
        case ColorRole: return f.colorHex;
        case TagsRole:  return f.tags;
    }
    return {};
}

QHash<int, QByteArray> FavoritesModel::roleNames() const {
    return {
        {IdRole,"favId"},{NameRole,"name"},{PathRole,"path"},
        {EmojiRole,"emoji"},{ColorRole,"colorHex"},{TagsRole,"tags"},
    };
}

void FavoritesModel::add(const QString& name, const QString& path, const QString& emoji) {
    beginInsertRows({}, items_.size(), items_.size());
    Favorite f;
    f.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    f.name = name; f.path = path; f.emoji = emoji;
    f.colorHex = "#0A84FF";
    items_.append(f);
    endInsertRows();
    persist();
}

void FavoritesModel::remove(const QString& id) {
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == id) {
            beginRemoveRows({}, i, i);
            items_.removeAt(i);
            endRemoveRows();
            persist();
            return;
        }
    }
}

QString FavoritesModel::pathOf(const QString& id) const {
    for (const auto& f : items_) if (f.id == id) return f.path;
    return {};
}

void FavoritesModel::persist() {
    QJsonArray arr;
    for (const auto& f : items_) {
        QJsonArray tags;
        for (const auto& t : f.tags) tags.append(t);
        arr.append(QJsonObject{
            {"id", f.id}, {"name", f.name}, {"path", f.path},
            {"emoji", f.emoji}, {"colorHex", f.colorHex}, {"tags", tags},
        });
    }
    store_->scheduleWrite(QJsonDocument(arr));
}

} // namespace dante
