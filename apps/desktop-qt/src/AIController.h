// AI Assistant overlay controller — Qt analogue of the Swift `ExplainModalState`
// + the AI launcher pieces. Exposed to QML as the `ai` context property.
//
// Responsibilities:
//   • Hold the visible/hidden state of the floating overlay.
//   • Append user/assistant/system messages to an in-memory transcript.
//   • Bridge to dante::ai::GroqClient for the actual chat round-trip.
//   • Persist the last 20 messages to %APPDATA%/Dante CLI/ai-history.json
//     via the existing JsonStore (debounced, atomic-rename writes).
//   • Surface common terminal-centric flows: explainCommand / suggestCommand
//     / insertIntoTerminal.
//
// QML wires `insertRequested(text)` to `terminal.write(appState.activeTabId,
// text)`, so this class never has to know about the terminal bridge.
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace dante {

class AppState;

namespace ai { class GroqClient; }
namespace persistence { class JsonStore; }

class AIController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool         open     READ open     NOTIFY openChanged)
    Q_PROPERTY(QString      status   READ status   NOTIFY statusChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QString      lastError READ lastError NOTIFY statusChanged)
public:
    explicit AIController(AppState* appState, QObject* parent = nullptr);
    ~AIController() override;

    bool open() const { return open_; }
    QString status() const { return status_; }
    QVariantList messages() const { return messages_; }
    QString lastError() const { return lastError_; }

    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void clearHistory();

    /// Append the user's prompt, fire the chat request, append assistant
    /// reply when it arrives. No-op while a request is in-flight.
    Q_INVOKABLE void send(const QString& prompt);

    /// "Explique este comando do terminal de forma curta e útil." + cmd.
    /// Opens the overlay if it's closed.
    Q_INVOKABLE void explainCommand(const QString& cmd);

    /// "Sugira um comando" + goal. Opens the overlay if it's closed.
    Q_INVOKABLE void suggestCommand(const QString& goal);

    /// Ask QML to inject `text` into the active terminal session.
    Q_INVOKABLE void insertIntoTerminal(const QString& text);

signals:
    void openChanged();
    void statusChanged();
    void messagesChanged();

    /// QML connects this to `terminal.write(appState.activeTabId, text)`.
    void insertRequested(const QString& text);

private slots:
    void onChatFinished(const QString& content);
    void onChatFailed(const QString& reason);

private:
    void setOpen(bool v);
    void setStatus(const QString& s);
    void setLastError(const QString& s);

    void appendMessage(const QString& role, const QString& content);
    void persist();
    void hydrate();

    /// Sync the Groq client's apiKey from AppState. Updates `status` to
    /// "no_api_key" if AppState's key is empty.
    void refreshApiKey();

    AppState*        appState_{nullptr};
    ai::GroqClient*  groq_{nullptr};
    std::unique_ptr<persistence::JsonStore> historyStore_;

    bool          open_{false};
    QString       status_{"idle"};   // idle | thinking | error | no_api_key
    QString       lastError_;
    QVariantList  messages_;
    bool          inFlight_{false};
    QString       pendingSystem_;    // system prompt used for the active request
};

} // namespace dante
