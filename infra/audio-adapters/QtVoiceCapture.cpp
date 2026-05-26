#include "QtVoiceCapture.h"

#include <QAudioSource>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace dante::audio {

namespace {
    constexpr int kSampleRate    = 16000;
    constexpr int kChannels      = 1;
    constexpr int kBitsPerSample = 16;
    constexpr int kLevelMinIntervalMs = 33; // ~30 Hz

    /// Smoothing factor for the level EMA (0..1; lower = smoother).
    constexpr float kLevelAlpha = 0.35f;
}

QtVoiceCapture::QtVoiceCapture(QObject* parent)
    : IVoiceCapture(parent)
{
    format_.setSampleRate(kSampleRate);
    format_.setChannelCount(kChannels);
    format_.setSampleFormat(QAudioFormat::Int16);
}

QtVoiceCapture::~QtVoiceCapture() {
    if (recording_) cancel();
}

bool QtVoiceCapture::start(const QString& outputWavPath) {
    if (recording_) {
        // Defensive: caller asked to start while still capturing. Tear down
        // the old session first rather than leak it.
        cancel();
    }

    const QAudioDevice dev = QMediaDevices::defaultAudioInput();
    if (dev.isNull()) {
        emit error("Nenhum microfone disponível");
        return false;
    }

    // Negotiate the format. If the device doesn't natively accept our PCM
    // mono 16 kHz request, QAudioSource will silently resample on most
    // platforms in Qt 6.5+. We log but proceed.
    QAudioFormat fmt = format_;
    if (!dev.isFormatSupported(fmt)) {
        const QAudioFormat preferred = dev.preferredFormat();
        // Keep channel count & sample rate; only fall back on sample type
        // if Int16 truly isn't supported. (Whisper accepts float wav too
        // but our header writer is hardcoded for int16 — easier to keep.)
        if (preferred.sampleFormat() != QAudioFormat::Int16) {
            // Try Int16 anyway — Qt will resample.
        }
    }

    wavPath_   = outputWavPath;
    dataBytes_ = 0;
    level_.store(0.0f, std::memory_order_relaxed);
    lastLevelEmitMs_ = 0;

    file_ = new QFile(wavPath_, this);
    if (!file_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit error(QStringLiteral("Não consegui criar arquivo: %1").arg(wavPath_));
        delete file_; file_ = nullptr;
        return false;
    }
    file_->write(makeWavHeader(kSampleRate, kChannels, kBitsPerSample));

    source_ = new QAudioSource(dev, fmt, this);
    // ~50 ms buffer keeps latency low while still giving us enough samples
    // per RMS window. (50 ms @ 16 kHz mono s16 = 1600 bytes.)
    source_->setBufferSize(kSampleRate * kChannels * (kBitsPerSample/8) / 20);

    io_ = source_->start();
    if (!io_) {
        emit error("Falha ao iniciar captura de áudio (permissão negada?)");
        file_->close();
        deletePartial();
        delete file_; file_ = nullptr;
        delete source_; source_ = nullptr;
        return false;
    }

    connect(io_, &QIODevice::readyRead, this, &QtVoiceCapture::onAudioReady);

    recording_ = true;
    return true;
}

void QtVoiceCapture::onAudioReady() {
    if (!recording_ || !io_ || !file_) return;

    const QByteArray chunk = io_->readAll();
    if (chunk.isEmpty()) return;

    file_->write(chunk);
    dataBytes_ += chunk.size();

    // ── RMS on the freshly-arrived int16 samples ──
    const auto* samples = reinterpret_cast<const qint16*>(chunk.constData());
    const int nSamples = chunk.size() / int(sizeof(qint16));
    if (nSamples > 0) {
        double sumSq = 0.0;
        for (int i = 0; i < nSamples; ++i) {
            const double s = double(samples[i]) / 32768.0;
            sumSq += s * s;
        }
        const float rms  = float(std::sqrt(sumSq / nSamples));
        // Perceptual squash: voices peak around 0.1..0.3 RMS in practice;
        // map sqrt so the UI bars look lively without clipping.
        const float norm = std::clamp(std::sqrt(rms) * 1.5f, 0.0f, 1.0f);

        const float prev = level_.load(std::memory_order_relaxed);
        const float smoothed = prev + (norm - prev) * kLevelAlpha;
        level_.store(smoothed, std::memory_order_relaxed);

        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - lastLevelEmitMs_ >= kLevelMinIntervalMs) {
            lastLevelEmitMs_ = nowMs;
            emit levelChanged(smoothed);
        }
    }
}

void QtVoiceCapture::stop() {
    if (!recording_) return;
    recording_ = false;

    if (source_) {
        source_->stop();
        source_->deleteLater();
        source_ = nullptr;
    }
    io_.clear();

    finalizeWav();

    const QString path = wavPath_;
    wavPath_.clear();
    level_.store(0.0f, std::memory_order_relaxed);
    emit levelChanged(0.0f);
    emit recordingStopped(path);
}

void QtVoiceCapture::cancel() {
    if (!recording_) {
        // Even when not recording, scrub a leftover partial (start() failed).
        if (file_) { file_->close(); delete file_; file_ = nullptr; }
        if (!wavPath_.isEmpty()) deletePartial();
        return;
    }
    recording_ = false;

    if (source_) {
        source_->stop();
        source_->deleteLater();
        source_ = nullptr;
    }
    io_.clear();

    if (file_) {
        file_->close();
        delete file_;
        file_ = nullptr;
    }
    deletePartial();
    wavPath_.clear();
    level_.store(0.0f, std::memory_order_relaxed);
    emit levelChanged(0.0f);
}

void QtVoiceCapture::finalizeWav() {
    if (!file_) return;

    // Patch the two size fields in the RIFF header now that we know the
    // payload length. Offsets per the RIFF/WAVE spec:
    //   bytes  4.. 7 → ChunkSize    = 36 + dataBytes_
    //   bytes 40..43 → Subchunk2Size = dataBytes_
    const quint32 chunkSize = quint32(36 + dataBytes_);
    const quint32 dataSize  = quint32(dataBytes_);

    QByteArray cs(4, '\0'); qToLittleEndian<quint32>(chunkSize, cs.data());
    QByteArray ds(4, '\0'); qToLittleEndian<quint32>(dataSize,  ds.data());

    file_->seek(4);  file_->write(cs);
    file_->seek(40); file_->write(ds);
    file_->flush();
    file_->close();
    delete file_;
    file_ = nullptr;
}

void QtVoiceCapture::deletePartial() {
    if (wavPath_.isEmpty()) return;
    QFile::remove(wavPath_);
}

QByteArray QtVoiceCapture::makeWavHeader(int sampleRate, int channels, int bitsPerSample) {
    QByteArray h(44, '\0');
    char* p = h.data();

    const quint16 blockAlign   = quint16(channels * (bitsPerSample / 8));
    const quint32 byteRate     = quint32(sampleRate) * blockAlign;

    // "RIFF" <size> "WAVE"
    std::memcpy(p +  0, "RIFF", 4);
    qToLittleEndian<quint32>(0, p + 4);            // ChunkSize — patched
    std::memcpy(p +  8, "WAVE", 4);

    // "fmt " subchunk (16 bytes, PCM)
    std::memcpy(p + 12, "fmt ", 4);
    qToLittleEndian<quint32>(16,                       p + 16); // Subchunk1Size
    qToLittleEndian<quint16>(1,                        p + 20); // AudioFormat = PCM
    qToLittleEndian<quint16>(quint16(channels),        p + 22);
    qToLittleEndian<quint32>(quint32(sampleRate),      p + 24);
    qToLittleEndian<quint32>(byteRate,                 p + 28);
    qToLittleEndian<quint16>(blockAlign,               p + 32);
    qToLittleEndian<quint16>(quint16(bitsPerSample),   p + 34);

    // "data" subchunk
    std::memcpy(p + 36, "data", 4);
    qToLittleEndian<quint32>(0, p + 40);           // Subchunk2Size — patched

    return h;
}

} // namespace dante::audio
