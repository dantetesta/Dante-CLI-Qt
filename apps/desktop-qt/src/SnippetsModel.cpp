#include "SnippetsModel.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
namespace dante {

SnippetsModel::SnippetsModel(QObject* parent)
    : QAbstractListModel(parent)
    , store_(std::make_unique<persistence::JsonStore>("snippets.json", 300, this))
{}

void SnippetsModel::hydrate() {
    beginResetModel();
    items_.clear();
    for (const auto& v : store_->read({}).array()) {
        const auto o = v.toObject();
        Snippet s;
        s.id = o.value("id").toString();
        s.name = o.value("name").toString();
        s.command = o.value("command").toString();
        s.emoji = o.value("emoji").toString();
        items_.append(s);
    }
    endResetModel();
}

int SnippetsModel::rowCount(const QModelIndex& p) const { return p.isValid() ? 0 : items_.size(); }

QVariant SnippetsModel::data(const QModelIndex& idx, int role) const {
    if (idx.row() < 0 || idx.row() >= items_.size()) return {};
    const auto& s = items_.at(idx.row());
    switch (role) {
        case IdRole: return s.id;
        case NameRole: return s.name;
        case CommandRole: return s.command;
        case EmojiRole: return s.emoji;
    }
    return {};
}

QHash<int, QByteArray> SnippetsModel::roleNames() const {
    return {{IdRole,"snipId"},{NameRole,"name"},{CommandRole,"command"},{EmojiRole,"emoji"}};
}

void SnippetsModel::add(const QString& name, const QString& command, const QString& emoji) {
    beginInsertRows({}, items_.size(), items_.size());
    Snippet s; s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.name = name; s.command = command; s.emoji = emoji;
    items_.append(s); endInsertRows(); persist();
}

void SnippetsModel::remove(const QString& id) {
    for (int i = 0; i < items_.size(); ++i)
        if (items_[i].id == id) {
            beginRemoveRows({}, i, i); items_.removeAt(i);
            endRemoveRows(); persist(); return;
        }
}

QString SnippetsModel::commandOf(const QString& id) const {
    for (const auto& s : items_) if (s.id == id) return s.command;
    return {};
}

void SnippetsModel::persist() {
    QJsonArray arr;
    for (const auto& s : items_) {
        arr.append(QJsonObject{
            {"id", s.id}, {"name", s.name},
            {"command", s.command}, {"emoji", s.emoji},
        });
    }
    store_->scheduleWrite(QJsonDocument(arr));
}

} // namespace dante
