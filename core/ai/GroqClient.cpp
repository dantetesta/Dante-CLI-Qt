#include "GroqClient.h"
#include <memory>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>

namespace dante::ai {

namespace {
    constexpr auto kChatUrl    = "https://api.groq.com/openai/v1/chat/completions";
    constexpr auto kWhisperUrl = "https://api.groq.com/openai/v1/audio/transcriptions";
}

GroqClient::GroqClient(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{}

void GroqClient::chat(const QString& system, const QString& user) {
    if (apiKey_.isEmpty()) {
        emit requestFailed("Groq API key missing");
        return;
    }
    QJsonObject body{
        {"model", chatModel_},
        {"temperature", 0.3},
        {"max_tokens", 1500},
        {"messages", QJsonArray{
            QJsonObject{{"role","system"},{"content",system}},
            QJsonObject{{"role","user"},  {"content",user}},
        }},
    };
    QNetworkRequest req((QUrl(kChatUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + apiKey_).toUtf8());
    req.setTransferTimeout(30000);

    auto* reply = nam_->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QString("Chat HTTP %1: %2")
                .arg(int(reply->error())).arg(reply->errorString()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto choices = doc.object().value("choices").toArray();
        if (choices.isEmpty()) { emit requestFailed("Empty choices"); return; }
        const auto msg = choices.first().toObject().value("message").toObject();
        emit chatFinished(msg.value("content").toString());
    });
}

void GroqClient::chatStream(const QString& system, const QString& user) {
    if (apiKey_.isEmpty()) {
        emit requestFailed("Groq API key missing");
        return;
    }
    QJsonObject body{
        {"model", chatModel_},
        {"temperature", 0.3},
        {"max_tokens", 1500},
        {"stream", true},
        {"messages", QJsonArray{
            QJsonObject{{"role","system"},{"content",system}},
            QJsonObject{{"role","user"},  {"content",user}},
        }},
    };
    QNetworkRequest req((QUrl(kChatUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + apiKey_).toUtf8());
    req.setRawHeader("Accept", "text/event-stream");
    // Disable transfer timeout — streams stay open for the whole response.
    req.setTransferTimeout(0);

    auto* reply = nam_->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    // SSE parser state — accumulates partial lines across readyRead calls.
    // shared_ptr is captured by value into both lambdas; they share the same
    // buffer and it dies when the last lambda goes out of scope (after the
    // reply finishes and both lambdas are disconnected).
    auto buffer = std::make_shared<QByteArray>();

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, buffer]() {
        buffer->append(reply->readAll());
        for (;;) {
            const int nl = buffer->indexOf('\n');
            if (nl < 0) break;
            const QByteArray line = buffer->left(nl).trimmed();
            buffer->remove(0, nl + 1);
            if (line.isEmpty() || !line.startsWith("data:")) continue;
            const QByteArray payload = line.mid(5).trimmed();
            if (payload == "[DONE]") continue;   // sentinel — handled in finished
            const auto doc = QJsonDocument::fromJson(payload);
            const auto choices = doc.object().value("choices").toArray();
            if (choices.isEmpty()) continue;
            const auto delta = choices.first().toObject().value("delta").toObject();
            const QString content = delta.value("content").toString();
            if (!content.isEmpty()) emit chatChunk(content);
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QString("Chat-stream HTTP %1: %2")
                .arg(int(reply->error())).arg(reply->errorString()));
            return;
        }
        emit chatStreamFinished();
    });
}

void GroqClient::whisper(const QString& audioPath, const QString& language) {
    if (apiKey_.isEmpty()) {
        emit requestFailed("Groq API key missing");
        return;
    }
    auto* file = new QFile(audioPath);
    if (!file->open(QIODevice::ReadOnly)) {
        emit requestFailed("Cannot read audio file: " + audioPath);
        file->deleteLater();
        return;
    }

    auto* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant("form-data; name=\"file\"; filename=\"audio.wav\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    filePart.setBodyDevice(file);
    file->setParent(mp);
    mp->append(filePart);

    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"model\""));
    modelPart.setBody(whisperModel_.toUtf8());
    mp->append(modelPart);

    QHttpPart langPart;
    langPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"language\""));
    langPart.setBody(language.toUtf8());
    mp->append(langPart);

    QNetworkRequest req((QUrl(kWhisperUrl)));
    req.setRawHeader("Authorization", ("Bearer " + apiKey_).toUtf8());
    req.setTransferTimeout(180000);
    auto* reply = nam_->post(req, mp);
    mp->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QString("Whisper HTTP %1: %2")
                .arg(int(reply->error())).arg(reply->errorString()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        emit whisperFinished(doc.object().value("text").toString());
    });
}

} // namespace dante::ai
