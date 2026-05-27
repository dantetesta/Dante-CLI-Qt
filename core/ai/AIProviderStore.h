// Pure model store for AIProvider entries. No IO, no QNetworkAccessManager,
// no QML — that lives in the desktop-qt layer (AIProvidersModel).
//
// JSON shape (round-trips through (loadFromJson, toJson)):
//   [
//     { "id": "uuid", "name": "Groq", "baseUrl": "...",
//       "apiKeyRef": "<literal-key-or-empty>", "model": "...",
//       "kind": "both", "enabled": true },
//     ...
//   ]
//
// If `loadFromJson` is called with an empty array, the store falls back
// to a curated default list (Groq + OpenAI + OpenRouter) so the user has
// something useful to pick from on first launch.
#pragma once

#include "AIProvider.h"
#include <QJsonArray>
#include <QString>
#include <QVector>

namespace dante::ai {

class AIProviderStore {
public:
    AIProviderStore() = default;

    /// Replace the in-memory list with the parsed JSON entries. When the
    /// array is empty or contains zero valid entries, seeds the defaults.
    void loadFromJson(const QJsonArray& arr);

    /// Serialize the current list. Keys match the JSON shape documented
    /// at the top of this file.
    QJsonArray toJson() const;

    const QVector<AIProvider>& all() const { return items_; }

    /// Insert or update by id. Empty id means "treat as new" — caller is
    /// expected to assign a UUID before invoking.
    void upsert(const AIProvider& p);

    /// Remove the entry with `id`. No-op if not found.
    void remove(const QString& id);

    /// Lookup by id. Returns a default-constructed AIProvider (empty id)
    /// when no match is found.
    AIProvider find(const QString& id) const;

    /// Seed the default Groq / OpenAI / OpenRouter trio (used both by
    /// loadFromJson(empty) and by callers that want a "reset to defaults".
    static QVector<AIProvider> seededDefaults();

private:
    QVector<AIProvider> items_;
};

} // namespace dante::ai
