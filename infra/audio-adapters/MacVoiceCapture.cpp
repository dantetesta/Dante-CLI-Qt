#include "MacVoiceCapture.h"

#include <QDateTime>
#include <QFile>
#include <QPointer>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace dante::audio {

// Canonical 44-byte RIFF/WAVE header (mirrors QtVoiceCapture::makeWavHeader so
// downstream Whisper upload behaves identically). Sizes are zeroed; finalize
// rewrites bytes 4..7 and 40..43.
static QByteArray makeWavHeader(int sampleRate, int channels, int bitsPerSample) {
    QByteArray h(44, '\0');
    char* p = h.data();
    const quint16 blockAlign = quint16(channels * (bitsPerSample / 8));
    const quint32 byteRate   = quint32(sampleRate) * blockAlign;

    std::memcpy(p +  0, "RIFF", 4);
    qToLittleEndian<quint32>(0, p + 4);
    std::memcpy(p +  8, "WAVE", 4);
    std::memcpy(p + 12, "fmt ", 4);
    qToLittleEndian<quint32>(16,                     p + 16);
    qToLittleEndian<quint16>(1,                      p + 20); // PCM
    qToLittleEndian<quint16>(quint16(channels),      p + 22);
    qToLittleEndian<quint32>(quint32(sampleRate),    p + 24);
    qToLittleEndian<quint32>(byteRate,               p + 28);
    qToLittleEndian<quint16>(blockAlign,             p + 32);
    qToLittleEndian<quint16>(quint16(bitsPerSample), p + 34);
    std::memcpy(p + 36, "data", 4);
    qToLittleEndian<quint32>(0, p + 40);
    return h;
}

namespace {
    constexpr int   kSampleRate         = 16000;
    constexpr int   kChannels           = 1;
    constexpr int   kBitsPerSample      = 16;
    constexpr int   kLevelMinIntervalMs = 33;
    constexpr float kLevelAlpha         = 0.35f;
}

} // namespace dante::audio

// =====================================================================
// macOS / Objective-C++ implementation
// =====================================================================
#ifdef __APPLE__

#import <AVFoundation/AVFoundation.h>

namespace dante::audio {

struct MacVoiceCapture::Impl {
    AVAudioEngine*    engine     = nil;
    AVAudioConverter* converter  = nil;
    AVAudioFormat*    targetFmt  = nil;

    QFile*            file       = nullptr;
    QString           wavPath;
    qint64            dataBytes  = 0;
    qint64            lastLevelMs= 0;
};

MacVoiceCapture::MacVoiceCapture(QObject* parent)
    : IVoiceCapture(parent), d_(std::make_unique<Impl>()) {}

MacVoiceCapture::~MacVoiceCapture() {
    if (recording_.load(std::memory_order_acquire)) {
        cancel();
    }
}

bool MacVoiceCapture::start(const QString& outputWavPath) {
    if (recording_.load(std::memory_order_acquire)) {
        cancel();
    }

    // Microphone permission. requestAccessForMediaType is async; we block on
    // a semaphore so start() retains its boolean contract. UI calls start()
    // from the GUI thread but we never hold the main runloop more than a
    // single OS prompt — acceptable for a one-shot prime.
    __block BOOL granted = NO;
    AVAuthorizationStatus status =
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    if (status == AVAuthorizationStatusAuthorized) {
        granted = YES;
    } else if (status == AVAuthorizationStatusNotDetermined) {
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                                 completionHandler:^(BOOL ok) {
            granted = ok;
            dispatch_semaphore_signal(sem);
        }];
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    }
    if (!granted) {
        emit error(QStringLiteral("Permissão de microfone negada"));
        return false;
    }

    d_->wavPath   = outputWavPath;
    d_->dataBytes = 0;
    d_->lastLevelMs = 0;
    level_.store(0.0f, std::memory_order_relaxed);

    d_->file = new QFile(d_->wavPath);
    if (!d_->file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit error(QStringLiteral("Não consegui criar arquivo: %1").arg(d_->wavPath));
        delete d_->file; d_->file = nullptr;
        return false;
    }
    d_->file->write(makeWavHeader(kSampleRate, kChannels, kBitsPerSample));

    @autoreleasepool {
        d_->engine = [[AVAudioEngine alloc] init];
        AVAudioInputNode* input = [d_->engine inputNode];
        AVAudioFormat* nativeFmt = [input inputFormatForBus:0];

        // Target: 16 kHz mono Int16, interleaved, little-endian.
        AudioStreamBasicDescription asbd = {};
        asbd.mSampleRate       = double(kSampleRate);
        asbd.mFormatID         = kAudioFormatLinearPCM;
        asbd.mFormatFlags      = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        asbd.mBitsPerChannel   = kBitsPerSample;
        asbd.mChannelsPerFrame = kChannels;
        asbd.mFramesPerPacket  = 1;
        asbd.mBytesPerFrame    = (kBitsPerSample / 8) * kChannels;
        asbd.mBytesPerPacket   = asbd.mBytesPerFrame;
        d_->targetFmt = [[AVAudioFormat alloc] initWithStreamDescription:&asbd];

        d_->converter = [[AVAudioConverter alloc] initFromFormat:nativeFmt toFormat:d_->targetFmt];
        if (!d_->converter) {
            emit error(QStringLiteral("AVAudioConverter init falhou"));
            d_->file->close();
            QFile::remove(d_->wavPath);
            delete d_->file; d_->file = nullptr;
            d_->engine = nil;
            d_->targetFmt = nil;
            return false;
        }

        QPointer<MacVoiceCapture> weak = this;
        // 4096-frame tap is plenty: at 48 kHz that's ~85 ms, well above the
        // 50 ms latency of QtVoiceCapture and still smooth for the meter.
        [input installTapOnBus:0
                    bufferSize:4096
                        format:nativeFmt
                         block:^(AVAudioPCMBuffer* buf, AVAudioTime* /*when*/) {
            MacVoiceCapture* self = weak.data();
            if (!self || !self->recording_.load(std::memory_order_acquire)) return;

            // Output capacity scaled by the rate ratio (round up).
            const AVAudioFrameCount inFrames = buf.frameLength;
            const double ratio = double(kSampleRate) / nativeFmt.sampleRate;
            const AVAudioFrameCount outCap =
                AVAudioFrameCount(std::ceil(double(inFrames) * ratio)) + 32;

            AVAudioPCMBuffer* outBuf =
                [[AVAudioPCMBuffer alloc] initWithPCMFormat:self->d_->targetFmt
                                              frameCapacity:outCap];
            if (!outBuf) return;

            __block BOOL fed = NO;
            AVAudioConverterInputBlock inBlock = ^AVAudioBuffer*(AVAudioPacketCount /*nPackets*/,
                                                                 AVAudioConverterInputStatus* status) {
                if (fed) { *status = AVAudioConverterInputStatus_NoDataNow; return nil; }
                fed = YES;
                *status = AVAudioConverterInputStatus_HaveData;
                return buf;
            };

            NSError* err = nil;
            AVAudioConverterOutputStatus st =
                [self->d_->converter convertToBuffer:outBuf error:&err withInputFromBlock:inBlock];
            if (st == AVAudioConverterOutputStatus_Error || err) {
                return;
            }

            const AVAudioFrameCount outFrames = outBuf.frameLength;
            if (outFrames == 0) return;

            const qint64 bytes = qint64(outFrames) * (kBitsPerSample / 8) * kChannels;
            const char* src = reinterpret_cast<const char*>(outBuf.int16ChannelData[0]);

            // File I/O is fine off the main thread; QFile is reentrant
            // per-instance and only this tap callback writes.
            if (self->d_->file) {
                self->d_->file->write(src, bytes);
                self->d_->dataBytes += bytes;
            }

            // RMS on the freshly-resampled int16 frames.
            const qint16* samples = reinterpret_cast<const qint16*>(src);
            double sumSq = 0.0;
            for (AVAudioFrameCount i = 0; i < outFrames; ++i) {
                const double s = double(samples[i]) / 32768.0;
                sumSq += s * s;
            }
            const float rms  = float(std::sqrt(sumSq / double(outFrames)));
            const float norm = std::clamp(std::sqrt(rms) * 1.5f, 0.0f, 1.0f);
            const float prev = self->level_.load(std::memory_order_relaxed);
            const float sm   = prev + (norm - prev) * kLevelAlpha;
            self->level_.store(sm, std::memory_order_relaxed);

            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            if (nowMs - self->d_->lastLevelMs >= kLevelMinIntervalMs) {
                self->d_->lastLevelMs = nowMs;
                // Hop to the owning thread before touching Qt signals.
                QMetaObject::invokeMethod(self, [self, sm]() {
                    emit self->levelChanged(sm);
                }, Qt::QueuedConnection);
            }
        }];

        [d_->engine prepare];
        NSError* startErr = nil;
        if (![d_->engine startAndReturnError:&startErr]) {
            const QString msg = startErr
                ? QString::fromNSString(startErr.localizedDescription)
                : QStringLiteral("desconhecido");
            emit error(QStringLiteral("AVAudioEngine falhou: %1").arg(msg));
            [input removeTapOnBus:0];
            d_->engine = nil;
            d_->converter = nil;
            d_->targetFmt = nil;
            d_->file->close();
            QFile::remove(d_->wavPath);
            delete d_->file; d_->file = nullptr;
            return false;
        }
    }

    recording_.store(true, std::memory_order_release);
    return true;
}

void MacVoiceCapture::stop() {
    if (!recording_.load(std::memory_order_acquire)) return;
    recording_.store(false, std::memory_order_release);

    @autoreleasepool {
        if (d_->engine) {
            [[d_->engine inputNode] removeTapOnBus:0];
            [d_->engine stop];
            d_->engine = nil;
        }
        d_->converter = nil;
        d_->targetFmt = nil;
    }

    if (d_->file) {
        const quint32 chunkSize = quint32(36 + d_->dataBytes);
        const quint32 dataSize  = quint32(d_->dataBytes);
        QByteArray cs(4, '\0'); qToLittleEndian<quint32>(chunkSize, cs.data());
        QByteArray ds(4, '\0'); qToLittleEndian<quint32>(dataSize,  ds.data());
        d_->file->seek(4);  d_->file->write(cs);
        d_->file->seek(40); d_->file->write(ds);
        d_->file->flush();
        d_->file->close();
        delete d_->file; d_->file = nullptr;
    }

    const QString path = d_->wavPath;
    d_->wavPath.clear();
    level_.store(0.0f, std::memory_order_relaxed);
    emit levelChanged(0.0f);
    emit recordingStopped(path);
}

void MacVoiceCapture::cancel() {
    const bool wasRecording = recording_.exchange(false, std::memory_order_acq_rel);

    @autoreleasepool {
        if (d_->engine) {
            [[d_->engine inputNode] removeTapOnBus:0];
            [d_->engine stop];
            d_->engine = nil;
        }
        d_->converter = nil;
        d_->targetFmt = nil;
    }

    if (d_->file) {
        d_->file->close();
        delete d_->file; d_->file = nullptr;
    }
    if (!d_->wavPath.isEmpty()) {
        QFile::remove(d_->wavPath);
        d_->wavPath.clear();
    }
    if (wasRecording) {
        level_.store(0.0f, std::memory_order_relaxed);
        emit levelChanged(0.0f);
    }
}

} // namespace dante::audio

// =====================================================================
// Non-Apple stub — file is still compiled cross-platform; keep it inert.
// =====================================================================
#else

namespace dante::audio {

struct MacVoiceCapture::Impl {};

MacVoiceCapture::MacVoiceCapture(QObject* parent)
    : IVoiceCapture(parent), d_(std::make_unique<Impl>()) {}
MacVoiceCapture::~MacVoiceCapture() = default;

bool MacVoiceCapture::start(const QString& /*outputWavPath*/) {
    emit error(QStringLiteral("MacVoiceCapture só está disponível no macOS."));
    return false;
}
void MacVoiceCapture::stop()   {}
void MacVoiceCapture::cancel() {}

} // namespace dante::audio

#endif // __APPLE__
