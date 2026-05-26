// HTTP client for Groq Chat completions + Whisper transcription.
// Mirrors `Services/GroqChatClient.swift` + `GroqWhisperClient.swift`.
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;

namespace dante::ai {

class GroqClient : public QObject {
    Q_OBJECT
public:
    explicit GroqClient(QObject* parent = nullptr);

    void setApiKey(QString k) { apiKey_ = std::move(k); }
    void setChatModel(QString m)    { chatModel_ = std::move(m); }
    void setWhisperModel(QString m) { whisperModel_ = std::move(m); }

    /// Single-turn chat completion. Emits `chatFinished` with the response
    /// or `requestFailed` with the error.
    Q_INVOKABLE void chat(const QString& system, const QString& user);

    /// Whisper transcription. Reads `audioPath` from disk and uploads as
    /// multipart/form-data.
    Q_INVOKABLE void whisper(const QString& audioPath, const QString& language);

signals:
    void chatFinished(const QString& content);
    void whisperFinished(const QString& text);
    void requestFailed(const QString& reason);

private:
    QNetworkAccessManager* nam_;
    QString apiKey_;
    QString chatModel_{"llama-3.3-70b-versatile"};
    QString whisperModel_{"whisper-large-v3-turbo"};
};

} // namespace dante::ai
