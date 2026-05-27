#include "AIProvidersModel.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QVariantMap>

namespace dante {

namespace {
    constexpr auto kFilename   = "ai-providers.json";
    constexpr int  kDebounceMs = 500;
}

AIProvidersModel::AIProvidersModel(QObject* parent)
    : QAbstractListModel(parent)
    // 0ms internal debounce on the JsonStore — we own a separate QTimer at
    // the model level so multi-field edits coalesce inside one write tick.
    , file_(std::make_unique<persistence::JsonStore>(kFilename, 0, this))
{
    debounce_.setSingleShot(true);
    debounce_.setInterval(kDebounceMs);
    connect(&debounce_, &QTimer::timeout, this, &AIProvidersModel::persistNow);
}

void AIProvidersModel::hydrate() {
    beginResetModel();

    // The file may either be a bare JSON array (legacy / forward-compat)
    // or an object with "providers" + "defaultChat" + "defaultWhisper".
    // Detect by inspecting the root document type.
    const auto doc = file_->read({});
    QJsonArray providersArr;
    QString    chatId, whisperId;
    if (doc.isArray()) {
        providersArr = doc.array();
    } else {
        const auto root = doc.object();
        providersArr   = root.value("providers").toArray();
        chatId         = root.value("defaultChat").toString();
        whisperId      = root.value("defaultWhisper").toString();
    }

    store_.loadFromJson(providersArr);

    // Validate the cached default ids — if the user deleted the provider
    // (or hand-edited the file), fall back to "no default" rather than
    // referencing a ghost row.
    auto idExists = [this](const QString& id) {
        if (id.isEmpty()) return false;
        for (const auto& p : store_.all()) if (p.id == id) return true;
        return false;
    };
    defaultChatId_    = idExists(chatId)    ? chatId    : QString{};
    defaultWhisperId_ = idExists(whisperId) ? whisperId : QString{};

    endResetModel();
    emit defaultsChanged();
}

int AIProvidersModel::rowCount(const QModelIndex& p) const {
    return p.isValid() ? 0 : store_.all().size();
}

QVariant AIProvidersModel::data(const QModelIndex& idx, int role) const {
    const auto& items = store_.all();
    if (idx.row() < 0 || idx.row() >= items.size()) return {};
    return roleValueFor(items.at(idx.row()), role);
}

QVariant AIProvidersModel::roleValueFor(const ai::AIProvider& p, int role) const {
    switch (role) {
        case IdRole:               return p.id;
        case NameRole:             return p.name;
        case BaseUrlRole:          return p.baseUrl;
        case ModelRole:            return p.model;
        case KindRole:             return p.kind;
        case EnabledRole:          return p.enabled;
        case HasApiKeyRole:        return !p.apiKeyRef.isEmpty();
        case IsDefaultChatRole:    return p.id == defaultChatId_;
        case IsDefaultWhisperRole: return p.id == defaultWhisperId_;
    }
    return {};
}

QHash<int, QByteArray> AIProvidersModel::roleNames() const {
    return {
        {IdRole,               "providerId"},
        {NameRole,             "name"},
        {BaseUrlRole,          "baseUrl"},
        {ModelRole,            "model"},
        {KindRole,             "kind"},
        {EnabledRole,          "enabled"},
        {HasApiKeyRole,        "hasApiKey"},
        {IsDefaultChatRole,    "isDefaultChat"},
        {IsDefaultWhisperRole, "isDefaultWhisper"},
    };
}

QString AIProvidersModel::addProvider() {
    ai::AIProvider p;
    p.id      = QUuid::createUuid().toString(QUuid::WithoutBraces);
    p.name    = QStringLiteral("Novo provider");
    p.baseUrl = "https://api.openai.com/v1";
    p.kind    = "chat";
    p.model   = "gpt-4o-mini";
    p.enabled = true;

    const int row = store_.all().size();
    beginInsertRows({}, row, row);
    store_.upsert(p);
    endInsertRows();
    schedulePersist();
    return p.id;
}

int AIProvidersModel::indexOf(const QString& id) const {
    const auto& items = store_.all();
    for (int i = 0; i < items.size(); ++i) if (items[i].id == id) return i;
    return -1;
}

void AIProvidersModel::updateProvider(const QString& id,
                                      const QString& name,
                                      const QString& baseUrl,
                                      const QString& apiKey,
                                      const QString& model,
                                      const QString& kind,
                                      bool enabled) {
    const int row = indexOf(id);
    if (row < 0) return;
    auto p       = store_.all().at(row);
    p.name       = name;
    p.baseUrl    = baseUrl;
    p.apiKeyRef  = apiKey;   // MVP: literal secret stored in apiKeyRef.
    p.model      = model;
    p.kind       = kind;
    p.enabled    = enabled;
    store_.upsert(p);

    const auto idx = index(row);
    emit dataChanged(idx, idx);
    schedulePersist();
}

void AIProvidersModel::removeProvider(const QString& id) {
    const int row = indexOf(id);
    if (row < 0) return;
    beginRemoveRows({}, row, row);
    store_.remove(id);
    endRemoveRows();

    // Clear default refs that pointed at the deleted row.
    bool defChanged = false;
    if (defaultChatId_ == id)    { defaultChatId_.clear();    defChanged = true; }
    if (defaultWhisperId_ == id) { defaultWhisperId_.clear(); defChanged = true; }
    if (defChanged) emit defaultsChanged();
    schedulePersist();
}

QVariantMap AIProvidersModel::get(const QString& id) const {
    const auto p = store_.find(id);
    if (p.id.isEmpty()) return {};
    return QVariantMap{
        {"id",         p.id},
        {"name",       p.name},
        {"baseUrl",    p.baseUrl},
        {"apiKey",     p.apiKeyRef},  // expose under "apiKey" for the editor.
        {"model",      p.model},
        {"kind",       p.kind},
        {"enabled",    p.enabled},
    };
}

void AIProvidersModel::pickDefaultChat(const QString& id) {
    if (defaultChatId_ == id) return;
    defaultChatId_ = id;
    emit defaultsChanged();
    // Notify every row so the "default" badge re-renders.
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1), { IsDefaultChatRole });
    schedulePersist();
}

void AIProvidersModel::pickDefaultWhisper(const QString& id) {
    if (defaultWhisperId_ == id) return;
    defaultWhisperId_ = id;
    emit defaultsChanged();
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1), { IsDefaultWhisperRole });
    schedulePersist();
}

void AIProvidersModel::schedulePersist() {
    debounce_.start();
}

void AIProvidersModel::persistNow() {
    QJsonObject root{
        {"providers",      store_.toJson()},
        {"defaultChat",    defaultChatId_},
        {"defaultWhisper", defaultWhisperId_},
    };
    file_->scheduleWrite(QJsonDocument(root));
}

} // namespace dante
