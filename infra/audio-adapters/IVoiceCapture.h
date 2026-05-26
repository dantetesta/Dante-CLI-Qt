// Abstract microphone-capture contract. Implementations target a specific
// OS audio API (WASAPI, AVFoundation) or — for v1 — wrap Qt Multimedia for
// cross-platform parity. The output is always a finalized mono 16-bit PCM
// .wav file at 16 kHz, suitable for direct upload via
// `dante::ai::GroqClient::whisper(path, lang)`.
//
// Lifecycle:
//   start(path)  → begin streaming microphone samples to `path` (header is
//                   written immediately, data length patched in stop()).
//   stop()       → finalize wav, emit `recordingStopped(path)`.
//   cancel()     → abort, delete partial file (no `recordingStopped`).
//
// RMS metering: `levelChanged(float 0..1)` ticks while recording (capped at
// roughly UI refresh rate inside the adapter) so QML can drive a waveform.
// `currentLevel()` is a polling alternative.
#pragma once

#include <QObject>
#include <QString>

namespace dante::audio {

class IVoiceCapture : public QObject {
    Q_OBJECT
public:
    explicit IVoiceCapture(QObject* parent = nullptr) : QObject(parent) {}
    ~IVoiceCapture() override = default;

    /// Begin recording into `outputWavPath`. Returns false if the device
    /// could not be opened (no mic, denied permission, busy, etc).
    virtual bool start(const QString& outputWavPath) = 0;

    /// Finalize the wav file and emit `recordingStopped`. No-op if not
    /// currently recording.
    virtual void stop() = 0;

    /// Abort, delete the partial file, do NOT emit `recordingStopped`.
    virtual void cancel() = 0;

    /// Most-recent normalized RMS level (0..1). Returns 0 when idle.
    virtual float currentLevel() const = 0;

    /// Whether the adapter is actively capturing.
    virtual bool isRecording() const = 0;

signals:
    /// Per-buffer RMS update (already smoothed; UI can render directly).
    void levelChanged(float level);

    /// Emitted from stop() once the wav file is fully written + closed.
    void recordingStopped(const QString& path);

    /// User-visible error message. Recording is no longer active by the
    /// time this fires (the adapter has already torn down its sinks).
    void error(const QString& reason);
};

} // namespace dante::audio
