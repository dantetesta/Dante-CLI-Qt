#include "CredentialsModel.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QUuid>

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
        c.notes = o.value("notes").toString();
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

static QVector<CredentialField> fieldsFromVariant(const QVariantList& list) {
    QVector<CredentialField> out;
    for (const auto& v : list) {
        const auto m = v.toMap();
        CredentialField f;
        f.label = m.value("label").toString();
        f.value = m.value("value").toString();
        f.masked = m.value("masked").toBool();
        out.append(f);
    }
    return out;
}

void CredentialsModel::add(const QString& name, int kind, const QString& emoji, const QVariantList& fields) {
    beginInsertRows({}, items_.size(), items_.size());
    Credential c;
    c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    c.name = name;
    c.kind = CredentialKind(qBound(0, kind, 4));
    c.emoji = emoji;
    c.fields = fieldsFromVariant(fields);
    items_.append(c);
    endInsertRows();
    persist();
}

void CredentialsModel::update(const QString& id, const QVariantMap& props) {
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id != id) continue;
        auto& c = items_[i];
        if (props.contains("name"))  c.name = props.value("name").toString();
        if (props.contains("kind"))  c.kind = CredentialKind(qBound(0, props.value("kind").toInt(), 4));
        if (props.contains("emoji")) c.emoji = props.value("emoji").toString();
        if (props.contains("notes")) c.notes = props.value("notes").toString();
        if (props.contains("fields")) c.fields = fieldsFromVariant(props.value("fields").toList());
        const auto idx = index(i);
        emit dataChanged(idx, idx);
        persist();
        return;
    }
}

QVariantMap CredentialsModel::get(const QString& id) const {
    for (const auto& c : items_) {
        if (c.id != id) continue;
        QVariantList fields;
        for (const auto& f : c.fields) {
            fields.append(QVariantMap{
                {"label", f.label}, {"value", f.value}, {"masked", f.masked},
            });
        }
        return QVariantMap{
            {"id", c.id}, {"name", c.name}, {"kind", int(c.kind)},
            {"emoji", c.emoji}, {"notes", c.notes}, {"fields", fields},
        };
    }
    return {};
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
            {"emoji", c.emoji}, {"notes", c.notes}, {"fields", fields},
        });
    }
    store_->scheduleWrite(QJsonDocument(arr));
}

} // namespace dante
