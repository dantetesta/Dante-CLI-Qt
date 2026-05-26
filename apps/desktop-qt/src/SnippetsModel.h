#pragma once
#include <QAbstractListModel>
#include "domain/Models.h"
#include "persistence/JsonStore.h"
#include <memory>
namespace dante {

class SnippetsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { IdRole = Qt::UserRole + 1, NameRole, CommandRole, EmojiRole };
    explicit SnippetsModel(QObject* parent = nullptr);
    void hydrate();
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void add(const QString& name, const QString& command, const QString& emoji);
    Q_INVOKABLE void remove(const QString& id);
    Q_INVOKABLE QString commandOf(const QString& id) const;
private:
    void persist();
    QVector<Snippet> items_;
    std::unique_ptr<persistence::JsonStore> store_;
};

} // namespace dante
