#include "VoiceController.h"

#include "AppState.h"
#include "ai/GroqClient.h"
#include "audio-adapters/QtVoiceCapture.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>

namespace dante {

VoiceController::VoiceController(AppState* appState,
                                 ai::GroqClient* groq,
                                 QObject* parent)
    : QObject(parent)
    , appState_(appState)
    , groq_(groq)
    , capture_(makeCapture(this))
{
    if (!groq_) {
        // Standalone path: instantiate our own client. AppState's API key
        // is pushed in below so the user only configures it once.
        groq_ = new ai::GroqClient(this);
        ownedGroq_ = true;
    }
    if (appState_) {
        groq_->setApiKey(appState_->groqApiKey());
        connect(appState_, &AppState::settingsChanged, this, [this]() {
            if (groq_) groq_->setApiKey(appState_->groqApiKey());
        });
    }

    connect(capture_, &audio::IVoiceCapture::levelChanged,      this, &VoiceController::onLevel);
    connect(capture_, &audio::IVoiceCapture::recordingStopped,  this, &VoiceController::onRecordingStopped);
    connect(capture_, &audio::IVoiceCapture::error,             this, &VoiceController::onCaptureError);

    connect(groq_, &ai::GroqClient::whisperFinished, this, &VoiceController::onWhisperFinished);
    connect(groq_, &ai::GroqClient::requestFailed,   this, &VoiceController::onWhisperFailed);
}

VoiceController::~VoiceController() {
    if (capture_ && capture_->isRecording()) capture_->cancel();
}

void VoiceController::startRecording() {
    if (recording_ || status_ == "transcribing") return;

    if (appState_ && appState_->groqApiKey().isEmpty()) {
        setLastError("Configure a API key do Groq em Settings → Voz.");
        setStatus("error");
        return;
    }

    const QString path = makeTempPath();
    lastWavPath_ = path;
    setLastError({});
    if (!capture_->start(path)) {
        // The adapter has already emitted error(); status will flip below.
        setStatus("error");
        return;
    }
    setRecording(true);
    setStatus("recording");
}

void VoiceController::stopRecording() {
    if (!recording_) return;
    setRecording(false);
    setStatus("transcribing");
    capture_->stop(); // → onRecordingStopped → groq_->whisper(...)
}

void VoiceController::cancelRecording() {
    if (recording_) {
        capture_->cancel();
        setRecording(false);
    }
    // Discard any partial path so the next stopRecording doesn't reuse it.
    if (!lastWavPath_.isEmpty()) {
        QFile::remove(lastWavPath_);
        lastWavPath_.clear();
    }
    setLastError({});
    setStatus("idle");
    setLevel(0.0f);
}

void VoiceController::toggleRecording() {
    if (recording_) stopRecording();
    else            startRecording();
}

/* ───────── capture signals ───────── */

void VoiceController::onLevel(float v) {
    setLevel(v);
}

void VoiceController::onRecordingStopped(const QString& path) {
    lastWavPath_ = path;
    if (!groq_) {
        setLastError("Cliente Groq indisponível");
        setStatus("error");
        return;
    }
    // Already in "transcribing" from stopRecording(); just fire the upload.
    groq_->whisper(path, language_);
}

void VoiceController::onCaptureError(const QString& reason) {
    setRecording(false);
    setLastError(reason);
    setStatus("error");
    setLevel(0.0f);
}

/* ───────── whisper signals ───────── */

void VoiceController::onWhisperFinished(const QString& text) {
    // Only react when *we* were the one in flight (the GroqClient is shared
    // with AIController; its chat() also routes through requestFailed but
    // its successful responses go to chatFinished, so this filter is just
    // for whisperFinished cross-talk — currently impossible, but cheap).
    if (status_ != "transcribing") return;

    // Clean up the temp wav regardless of whether the response was empty.
    if (!lastWavPath_.isEmpty()) {
        QFile::remove(lastWavPath_);
        lastWavPath_.clear();
    }

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        setLastError("Whisper retornou texto vazio");
        setStatus("error");
        return;
    }
    setStatus("idle");
    emit transcribed(trimmed);
}

void VoiceController::onWhisperFailed(const QString& reason) {
    if (status_ != "transcribing") return; // probably a chat() failure
    if (!lastWavPath_.isEmpty()) {
        QFile::remove(lastWavPath_);
        lastWavPath_.clear();
    }
    setLastError(reason);
    setStatus("error");
}

/* ───────── setters ───────── */

void VoiceController::setStatus(const QString& s) {
    if (status_ == s) return;
    status_ = s;
    emit statusChanged();
}

void VoiceController::setRecording(bool v) {
    if (recording_ == v) return;
    recording_ = v;
    emit recordingChanged();
}

void VoiceController::setLevel(float v) {
    // Float compare is fine here — coming straight from the adapter.
    if (qFuzzyCompare(level_ + 1.0f, v + 1.0f)) return;
    level_ = v;
    emit levelChanged();
}

void VoiceController::setLastError(const QString& s) {
    if (lastError_ == s) return;
    lastError_ = s;
    emit statusChanged();
}

/* ───────── factories ───────── */

audio::IVoiceCapture* VoiceController::makeCapture(QObject* parent) {
    // Per the v1 plan: Qt Multimedia on both platforms. WasapiVoiceCapture
    // / MacVoiceCapture are stubs intended for a future swap.
    return new audio::QtVoiceCapture(parent);
}

QString VoiceController::makeTempPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (dir.isEmpty()) dir = QDir::tempPath();
    const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QDir(dir).filePath(QStringLiteral("dante-voice-%1.wav").arg(uuid));
}

} // namespace dante
