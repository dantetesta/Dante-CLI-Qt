#pragma once
#include <QAbstractListModel>
#include <QVariantMap>
#include <QVariantList>
#include "domain/Models.h"
#include "persistence/JsonStore.h"
#include <memory>
namespace dante {

class CredentialsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { IdRole = Qt::UserRole + 1, NameRole, KindRole, EmojiRole, FieldCountRole };
    explicit CredentialsModel(QObject* parent = nullptr);
    void hydrate();
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void add(const QString& name, int kind, const QString& emoji, const QVariantList& fields);
    Q_INVOKABLE void update(const QString& id, const QVariantMap& props);
    Q_INVOKABLE QVariantMap get(const QString& id) const;
    Q_INVOKABLE void remove(const QString& id);
    Q_INVOKABLE QString renderInline(const QString& id) const;
private:
    void persist();
    QVector<Credential> items_;
    std::unique_ptr<persistence::JsonStore> store_;
};

} // namespace dante
