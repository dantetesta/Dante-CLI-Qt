#include "AIController.h"

#include "AppState.h"
#include "ai/GroqClient.h"
#include "persistence/JsonStore.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

namespace dante {

namespace {
    constexpr int  kMaxHistory = 20;
    constexpr auto kHistoryFile = "ai-history.json";

    constexpr auto kSystemPromptDefault =
        "Você é o assistente do Dante CLI — um terminal premium. Responda em "
        "português técnico, direto e claro. Prefira markdown leve: **negrito** "
        "para conceitos-chave e `código inline` para comandos/paths. Quando "
        "sugerir comandos prontos pra colar, use blocos ```. Sem floreios.";

    constexpr auto kSystemPromptExplain =
        "Explique este comando do terminal de forma curta e útil. Em português, "
        "tom técnico mas acessível. Diga (1) o que ele faz, (2) os argumentos "
        "relevantes, (3) alertas se for destrutivo. Use markdown leve e "
        "`código inline` para flags. Até 150 palavras.";

    constexpr auto kSystemPromptSuggest =
        "Sugira um comando de terminal (Windows PowerShell ou bash/zsh, escolha "
        "o que fizer mais sentido pra meta descrita) que atinja o objetivo do "
        "usuário. Responda em português, com o comando em bloco ``` e 1-2 "
        "linhas explicando. Se houver risco, avise antes do comando.";

    QString nowIsoUtc() {
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }
}

AIController::AIController(AppState* appState, QObject* parent)
    : QObject(parent)
    , appState_(appState)
    , groq_(new ai::GroqClient(this))
    , historyStore_(std::make_unique<persistence::JsonStore>(kHistoryFile, 400, this))
{
    connect(groq_, &ai::GroqClient::chatFinished,       this, &AIController::onChatFinished);
    connect(groq_, &ai::GroqClient::chatChunk,          this, &AIController::onChatChunk);
    connect(groq_, &ai::GroqClient::chatStreamFinished, this, &AIController::onChatStreamFinished);
    connect(groq_, &ai::GroqClient::requestFailed,      this, &AIController::onChatFailed);

    if (appState_) {
        connect(appState_, &AppState::settingsChanged, this, &AIController::refreshApiKey);
    }

    hydrate();
    refreshApiKey();
}

AIController::~AIController() {
    if (historyStore_) historyStore_->flush();
}

void AIController::show()   { setOpen(true); }
void AIController::hide()   { setOpen(false); }
void AIController::toggle() { setOpen(!open_); }

void AIController::clearHistory() {
    messages_.clear();
    emit messagesChanged();
    persist();
    setLastError({});
    setStatus(appState_ && appState_->groqApiKey().isEmpty() ? "no_api_key" : "idle");
}

void AIController::send(const QString& prompt) {
    const QString trimmed = prompt.trimmed();
    if (trimmed.isEmpty()) return;
    if (inFlight_) return;

    refreshApiKey();
    if (appState_ && appState_->groqApiKey().isEmpty()) {
        setStatus("no_api_key");
        return;
    }

    appendMessage("user", trimmed);
    pendingSystem_ = kSystemPromptDefault;
    inFlight_ = true;
    setLastError({});
    setStatus("thinking");
    groq_->chatStream(pendingSystem_, trimmed);
}

void AIController::explainCommand(const QString& cmd) {
    const QString trimmed = cmd.trimmed();
    if (trimmed.isEmpty()) return;
    setOpen(true);
    if (inFlight_) return;

    refreshApiKey();
    if (appState_ && appState_->groqApiKey().isEmpty()) {
        setStatus("no_api_key");
        return;
    }

    appendMessage("user", QStringLiteral("Explique: `%1`").arg(trimmed));
    pendingSystem_ = kSystemPromptExplain;
    inFlight_ = true;
    setLastError({});
    setStatus("thinking");
    groq_->chatStream(pendingSystem_, trimmed);
}

void AIController::suggestCommand(const QString& goal) {
    const QString trimmed = goal.trimmed();
    if (trimmed.isEmpty()) return;
    setOpen(true);
    if (inFlight_) return;

    refreshApiKey();
    if (appState_ && appState_->groqApiKey().isEmpty()) {
        setStatus("no_api_key");
        return;
    }

    appendMessage("user", QStringLiteral("Sugira um comando para: %1").arg(trimmed));
    pendingSystem_ = kSystemPromptSuggest;
    inFlight_ = true;
    setLastError({});
    setStatus("thinking");
    groq_->chatStream(pendingSystem_, trimmed);
}

void AIController::insertIntoTerminal(const QString& text) {
    if (text.isEmpty()) return;
    emit insertRequested(text);
}

/* ───────── private ───────── */

void AIController::onChatFinished(const QString& content) {
    // Non-streaming path (kept for chat() callers if anyone uses it directly).
    inFlight_ = false;
    appendMessage("assistant", content);
    setStatus("idle");
}

void AIController::onChatChunk(const QString& delta) {
    // First chunk of a stream → create the assistant message; subsequent
    // chunks → append to it in place and re-emit messagesChanged so QML
    // ListView re-renders the same row with the new content.
    if (!inFlight_) return;
    if (messages_.isEmpty() || messages_.last().toMap().value("role").toString() != "assistant") {
        QVariantMap m;
        m.insert("role", "assistant");
        m.insert("content", delta);
        m.insert("timestamp", nowIsoUtc());
        messages_.append(m);
    } else {
        auto m = messages_.last().toMap();
        m["content"] = m.value("content").toString() + delta;
        messages_[messages_.size() - 1] = m;
    }
    emit messagesChanged();
    // Don't persist on every chunk — wait for stream finish.
    setStatus("streaming");
}

void AIController::onChatStreamFinished() {
    inFlight_ = false;
    setStatus("idle");
    persist();
}

void AIController::onChatFailed(const QString& reason) {
    inFlight_ = false;
    setLastError(reason);
    appendMessage("system", QStringLiteral("Erro: %1").arg(reason));
    setStatus("error");
}

void AIController::setOpen(bool v) {
    if (open_ == v) return;
    open_ = v;
    emit openChanged();
}

void AIController::setStatus(const QString& s) {
    if (status_ == s) return;
    status_ = s;
    emit statusChanged();
}

void AIController::setLastError(const QString& s) {
    if (lastError_ == s) return;
    lastError_ = s;
    // statusChanged carries lastError per Q_PROPERTY declaration.
    emit statusChanged();
}

void AIController::appendMessage(const QString& role, const QString& content) {
    QVariantMap m;
    m.insert("role",      role);
    m.insert("content",   content);
    m.insert("timestamp", nowIsoUtc());
    messages_.append(m);

    // Trim to last kMaxHistory entries — older context isn't useful for a
    // single-turn chat and bloats the persisted file.
    while (messages_.size() > kMaxHistory) messages_.removeFirst();

    emit messagesChanged();
    persist();
}

void AIController::persist() {
    QJsonArray arr;
    for (const auto& v : messages_) {
        const auto m = v.toMap();
        arr.append(QJsonObject{
            {"role",      m.value("role").toString()},
            {"content",   m.value("content").toString()},
            {"timestamp", m.value("timestamp").toString()},
        });
    }
    historyStore_->scheduleWrite(QJsonDocument(QJsonObject{
        {"messages", arr},
    }));
}

void AIController::hydrate() {
    const auto doc = historyStore_->read({});
    const auto arr = doc.object().value("messages").toArray();
    messages_.clear();
    for (const auto& v : arr) {
        const auto o = v.toObject();
        QVariantMap m;
        m.insert("role",      o.value("role").toString());
        m.insert("content",   o.value("content").toString());
        m.insert("timestamp", o.value("timestamp").toString());
        messages_.append(m);
    }
    while (messages_.size() > kMaxHistory) messages_.removeFirst();
    emit messagesChanged();
}

void AIController::refreshApiKey() {
    if (!appState_) {
        setStatus("no_api_key");
        return;
    }
    const QString key = appState_->groqApiKey();
    groq_->setApiKey(key);
    if (key.isEmpty()) {
        setStatus("no_api_key");
    } else if (status_ == "no_api_key") {
        setStatus("idle");
    }
}

} // namespace dante
