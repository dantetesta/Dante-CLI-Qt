#include "CredentialsModel.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

namespace dante {

CredentialsModel::CredentialsModel(QObject* parent)
    : QAbstractListModel(parent)
    , store_(std::make_unique<persistence::JsonStore>("credentials.json", 300, this))
{}

void CredentialsModel::hydrate() {
    beginResetModel();
    items_.clear();
    for (const auto& v : store_->read({}).array()) {
        const auto o = v.toObject();
        Credential c;
        c.id = o.value("id").toString();
        c.name = o.value("name").toString();
        c.kind = CredentialKind(o.value("kind").toInt(0));
        c.emoji = o.value("emoji").toString();
        for (const auto& f : o.value("fields").toArray()) {
            CredentialField fld;
            const auto fo = f.toObject();
            fld.label = fo.value("label").toString();
            fld.value = fo.value("value").toString();
            fld.masked = fo.value("masked").toBool();
            c.fields.append(fld);
        }
        items_.append(c);
    }
    endResetModel();
}

int CredentialsModel::rowCount(const QModelIndex& p) const { return p.isValid() ? 0 : items_.size(); }

QVariant CredentialsModel::data(const QModelIndex& idx, int role) const {
    if (idx.row() < 0 || idx.row() >= items_.size()) return {};
    const auto& c = items_.at(idx.row());
    switch (role) {
        case IdRole: return c.id;
        case NameRole: return c.name;
        case KindRole: return int(c.kind);
        case EmojiRole: return c.emoji;
        case FieldCountRole: return c.fields.size();
    }
    return {};
}

QHash<int, QByteArray> CredentialsModel::roleNames() const {
    return {{IdRole,"credId"},{NameRole,"name"},{KindRole,"kind"},
            {EmojiRole,"emoji"},{FieldCountRole,"fieldCount"}};
}

void CredentialsModel::remove(const QString& id) {
    for (int i = 0; i < items_.size(); ++i)
        if (items_[i].id == id) {
            beginRemoveRows({}, i, i); items_.removeAt(i);
            endRemoveRows(); persist(); return;
        }
}

QString CredentialsModel::renderInline(const QString& id) const {
    for (const auto& c : items_) {
        if (c.id != id) continue;
        QStringList parts;
        for (const auto& f : c.fields) parts << f.label + "=" + f.value;
        return QString("# Credencial %1 — %2 — %3\n")
            .arg(int(c.kind)).arg(c.name).arg(parts.join(" · "));
    }
    return {};
}

void CredentialsModel::persist() {
    QJsonArray arr;
    for (const auto& c : items_) {
        QJsonArray fields;
        for (const auto& f : c.fields) {
            fields.append(QJsonObject{
                {"label", f.label}, {"value", f.value}, {"masked", f.masked},
            });
        }
        arr.append(QJsonObject{
            {"id", c.id}, {"name", c.name}, {"kind", int(c.kind)},
            {"emoji", c.emoji}, {"fields", fields},
        });
    }
    store_->scheduleWrite(QJsonDocument(arr));
}

} // namespace dante
