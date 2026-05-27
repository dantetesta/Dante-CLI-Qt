#include "AIProviderStore.h"

#include <QJsonObject>
#include <QUuid>

namespace dante::ai {

namespace {
    QString newId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}

QVector<AIProvider> AIProviderStore::seededDefaults() {
    // Stable ids so the seeded list survives reloads when the user hasn't
    // edited anything yet — also lets downstream code (AppState bridges)
    // pin a default-chat selection without re-resolving by name.
    AIProvider groq;
    groq.id        = "seed-groq";
    groq.name      = "Groq";
    groq.baseUrl   = "https://api.groq.com/openai/v1";
    groq.apiKeyRef = "";
    groq.model     = "llama-3.3-70b-versatile";
    groq.kind      = "both";   // chat + whisper
    groq.enabled   = true;

    AIProvider openai;
    openai.id        = "seed-openai";
    openai.name      = "OpenAI";
    openai.baseUrl   = "https://api.openai.com/v1";
    openai.apiKeyRef = "";
    openai.model     = "gpt-4o-mini";
    openai.kind      = "chat";
    openai.enabled   = true;

    AIProvider openrouter;
    openrouter.id        = "seed-openrouter";
    openrouter.name      = "OpenRouter";
    openrouter.baseUrl   = "https://openrouter.ai/api/v1";
    openrouter.apiKeyRef = "";
    openrouter.model     = "openrouter/auto";
    openrouter.kind      = "chat";
    openrouter.enabled   = true;

    return { groq, openai, openrouter };
}

void AIProviderStore::loadFromJson(const QJsonArray& arr) {
    items_.clear();
    items_.reserve(arr.size());
    for (const auto& v : arr) {
        const auto o = v.toObject();
        AIProvider p;
        p.id        = o.value("id").toString();
        p.name      = o.value("name").toString();
        p.baseUrl   = o.value("baseUrl").toString();
        p.apiKeyRef = o.value("apiKeyRef").toString();
        p.model     = o.value("model").toString();
        p.kind      = o.value("kind").toString("chat");
        p.enabled   = o.value("enabled").toBool(true);
        if (p.id.isEmpty()) p.id = newId();
        // Skip blatantly malformed entries (no name AND no baseUrl) so
        // garbage rows can't poison the picker.
        if (p.name.isEmpty() && p.baseUrl.isEmpty()) continue;
        items_.append(p);
    }
    if (items_.isEmpty()) items_ = seededDefaults();
}

QJsonArray AIProviderStore::toJson() const {
    QJsonArray arr;
    for (const auto& p : items_) {
        arr.append(QJsonObject{
            {"id",        p.id},
            {"name",      p.name},
            {"baseUrl",   p.baseUrl},
            {"apiKeyRef", p.apiKeyRef},
            {"model",     p.model},
            {"kind",      p.kind},
            {"enabled",   p.enabled},
        });
    }
    return arr;
}

void AIProviderStore::upsert(const AIProvider& p) {
    if (p.id.isEmpty()) return;
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == p.id) {
            items_[i] = p;
            return;
        }
    }
    items_.append(p);
}

void AIProviderStore::remove(const QString& id) {
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].id == id) {
            items_.removeAt(i);
            return;
        }
    }
}

AIProvider AIProviderStore::find(const QString& id) const {
    for (const auto& p : items_) if (p.id == id) return p;
    return {};
}

} // namespace dante::ai
