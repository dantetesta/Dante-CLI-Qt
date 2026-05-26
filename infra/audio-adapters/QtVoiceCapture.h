// Cross-platform mic capture via Qt 6 Multimedia's QAudioSource.
//
// We deliberately bypass QMediaRecorder because (a) it always wraps audio in
// an OS-default container (m4a/aac on macOS, mp4 on Windows) — fine for
// Whisper, but it makes per-buffer RMS metering awkward, and (b) it has no
// portable knob for forcing 16 kHz mono PCM. QAudioSource gives us the raw
// frames so we can write a Whisper-friendly WAV ourselves and compute RMS
// in-line for the waveform UI.
//
// Output format: PCM s16le, mono, 16 kHz. RIFF/WAVE header written on
// start(); the 4-byte chunk-size + 4-byte data-size fields are patched in
// stop() once the final byte count is known.
#pragma once

#include "IVoiceCapture.h"

#include <QAudioFormat>
#include <QPointer>
#include <QString>
#include <QtGlobal>

#include <atomic>

class QAudioSource;
class QFile;
class QIODevice;

namespace dante::audio {

class QtVoiceCapture : public IVoiceCapture {
    Q_OBJECT
public:
    explicit QtVoiceCapture(QObject* parent = nullptr);
    ~QtVoiceCapture() override;

    bool  start(const QString& outputWavPath) override;
    void  stop() override;
    void  cancel() override;
    float currentLevel() const override { return level_.load(std::memory_order_relaxed); }
    bool  isRecording() const override  { return recording_; }

private:
    /// Pulls available bytes from the QIODevice the source pushes into,
    /// streams them to the WAV file, and computes a smoothed RMS level.
    void onAudioReady();

    /// Patch the WAV header's two size fields and close the file.
    void finalizeWav();

    /// Best-effort delete of `wavPath_`. Used by cancel() + start() retry.
    void deletePartial();

    /// 44-byte canonical RIFF/WAVE header for PCM s16le mono @ sampleRate.
    /// Sizes are placeholders (0xFFFFFFFF written as 0) — finalizeWav()
    /// rewinds + rewrites bytes 4..7 and 40..43 with real values.
    static QByteArray makeWavHeader(int sampleRate, int channels, int bitsPerSample);

    QAudioFormat   format_;
    QAudioSource*  source_   {nullptr};
    QPointer<QIODevice> io_;          // owned by source_, lifetime tracked here
    QFile*         file_     {nullptr};
    QString        wavPath_;
    qint64         dataBytes_{0};
    bool           recording_{false};

    // Filtered RMS, written from onAudioReady, read by UI thread via currentLevel().
    std::atomic<float> level_{0.0f};

    // Throttle levelChanged() emissions to ~30 Hz so we don't flood QML.
    qint64 lastLevelEmitMs_{0};
};

} // namespace dante::audio
