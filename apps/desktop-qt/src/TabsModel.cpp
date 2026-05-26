#include "TabsModel.h"
#include "AppState.h"

namespace dante {

TabsModel::TabsModel(AppState* state, QObject* parent)
    : QAbstractListModel(parent), state_(state)
{
    connect(state_, &AppState::tabsChanged, this, [this]() {
        beginResetModel(); endResetModel();
    });
    connect(state_, &AppState::activeTabIdChanged, this, [this]() {
        // The "active" role depends on AppState.activeTabId — refresh everyone.
        if (rowCount()) emit dataChanged(index(0), index(rowCount() - 1), {ActiveRole});
    });
}

int TabsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : state_->tabs().size();
}

QVariant TabsModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || idx.row() >= state_->tabs().size()) return {};
    const auto& t = state_->tabs().at(idx.row());
    switch (role) {
        case IdRole:     return t.id;
        case TitleRole:  return t.title;
        case ColorRole:  return t.color;
        case EmojiRole:  return t.emoji;
        case PinnedRole: return t.pinned;
        case ActiveRole: return t.id == state_->activeTabId();
        case CwdRole:    return t.cwd;
        case KindRole:   return int(t.kind);
    }
    return {};
}

QHash<int, QByteArray> TabsModel::roleNames() const {
    return {
        {IdRole,    "tabId"},
        {TitleRole, "title"},
        {ColorRole, "color"},
        {EmojiRole, "emoji"},
        {PinnedRole,"pinned"},
        {ActiveRole,"active"},
        {CwdRole,   "cwd"},
        {KindRole,  "kind"},
    };
}

} // namespace dante
