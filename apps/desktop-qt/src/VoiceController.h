// Voice-to-text orchestrator. Owns the platform IVoiceCapture adapter +
// reuses the existing dante::ai::GroqClient::whisper() endpoint to turn the
// captured WAV into text. Exposed to QML as the `voice` context property.
//
// State machine:
//   idle          ← initial; ready to record
//   recording     ← QAudioSource is feeding the WAV
//   transcribing  ← WAV closed, multipart upload in-flight
//   error         ← last operation failed; lastError holds the reason
//
// Output: emits `transcribed(text)` when Whisper returns a non-empty
// transcript. main.cpp wires this to TerminalBridge::write so the words
// appear in the active session — same UX as pasting the dictation.
#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

namespace dante {
class AppState;
namespace ai    { class GroqClient; }
namespace audio { class IVoiceCapture; }

class VoiceController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool    recording   READ recording   NOTIFY recordingChanged)
    Q_PROPERTY(float   level       READ level       NOTIFY levelChanged)
    Q_PROPERTY(QString status      READ status      NOTIFY statusChanged)
    Q_PROPERTY(QString lastError   READ lastError   NOTIFY statusChanged)
public:
    /// `groq` is borrowed (parent keeps ownership — typically AIController's
    /// instance, see main.cpp). If null, the controller spins up its own.
    explicit VoiceController(AppState* appState,
                             ai::GroqClient* groq,
                             QObject* parent = nullptr);
    ~VoiceController() override;

    bool    recording() const { return recording_; }
    float   level() const     { return level_; }
    QString status() const    { return status_; }
    QString lastError() const { return lastError_; }

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void cancelRecording();

    /// QML helper for the toolbar button — toggles between start and stop.
    Q_INVOKABLE void toggleRecording();

    /// Language hint passed to Whisper. Defaults to "pt".
    Q_INVOKABLE void setLanguage(const QString& lang) { language_ = lang; }

signals:
    void recordingChanged();
    void levelChanged();
    void statusChanged();

    /// Final transcribed text from Whisper. main.cpp routes this to the
    /// active terminal session (and/or the AI overlay's input).
    void transcribed(const QString& text);

private slots:
    void onLevel(float v);
    void onRecordingStopped(const QString& path);
    void onCaptureError(const QString& reason);
    void onWhisperFinished(const QString& text);
    void onWhisperFailed(const QString& reason);

private:
    void setStatus(const QString& s);
    void setRecording(bool v);
    void setLevel(float v);
    void setLastError(const QString& s);

    /// Picks the right adapter for the host OS. Currently always returns
    /// QtVoiceCapture — see the header notes in WasapiVoiceCapture /
    /// MacVoiceCapture for when we'd swap to the native paths.
    static audio::IVoiceCapture* makeCapture(QObject* parent);

    /// QStandardPaths::TempLocation + "/dante-voice-<uuid>.wav".
    static QString makeTempPath();

    AppState*               appState_{nullptr};
    QPointer<ai::GroqClient> groq_;        // borrowed unless ownedGroq_ is true
    bool                    ownedGroq_{false};
    audio::IVoiceCapture*   capture_{nullptr};

    bool    recording_{false};
    float   level_{0.0f};
    QString status_{"idle"};   // idle | recording | transcribing | error
    QString lastError_;
    QString language_{"pt"};
    QString lastWavPath_;
};

} // namespace dante
