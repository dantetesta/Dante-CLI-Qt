// Qt list model exposing AppState.tabs to QML's ListView.
#pragma once

#include <QAbstractListModel>

namespace dante {

class AppState;

class TabsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        ColorRole,
        EmojiRole,
        PinnedRole,
        ActiveRole,
        CwdRole,
        KindRole,
    };

    explicit TabsModel(AppState* state, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    AppState* state_;
};

} // namespace dante
