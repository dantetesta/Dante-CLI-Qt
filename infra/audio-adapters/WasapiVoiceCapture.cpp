#include "WasapiVoiceCapture.h"

#include <QDateTime>
#include <QFile>
#include <QPointer>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>

namespace dante::audio {

// Canonical 44-byte RIFF/WAVE header, matches QtVoiceCapture / MacVoiceCapture.
// Bytes 4..7 (ChunkSize) and 40..43 (Subchunk2Size) are patched on stop().
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
// Windows / WASAPI implementation
// =====================================================================
#ifdef _WIN32

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <avrt.h>
#include <combaseapi.h>
#include <objbase.h>

#include <thread>

namespace dante::audio {

namespace {

// Tiny RAII for COM pointers — avoid bringing in <wrl/client.h> just for this.
template <typename T>
struct ComPtr {
    T* p = nullptr;
    ComPtr() = default;
    ~ComPtr() { if (p) p->Release(); }
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    T** operator&() { return &p; }
    T* operator->() const { return p; }
    operator T*() const { return p; }
    void reset() { if (p) { p->Release(); p = nullptr; } }
};

struct HandleCloser {
    void operator()(HANDLE h) const noexcept {
        if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
};
using UniqueEvent = std::unique_ptr<void, HandleCloser>;

// Pull the IEEE-float flag out of a WAVEFORMATEX (handles WAVEFORMATEXTENSIBLE).
bool waveFormatIsFloat(const WAVEFORMATEX* wfx) {
    if (!wfx) return false;
    if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return true;
    if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);
        return IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) == TRUE;
    }
    return false;
}
bool waveFormatIsPcm(const WAVEFORMATEX* wfx) {
    if (!wfx) return false;
    if (wfx->wFormatTag == WAVE_FORMAT_PCM) return true;
    if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);
        return IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) == TRUE;
    }
    return false;
}

} // namespace

struct WasapiVoiceCapture::Impl {
    // COM/WASAPI handles. Released in reverse order of acquisition.
    ComPtr<IMMDeviceEnumerator>  enumerator;
    ComPtr<IMMDevice>            device;
    ComPtr<IAudioClient>         client;
    ComPtr<IAudioCaptureClient>  capture;
    WAVEFORMATEX*                mixFormat = nullptr;
    UniqueEvent                  bufferEvent;

    std::thread                  worker;
    std::atomic<bool>            shouldRun{false};

    QFile*                       file = nullptr;
    QString                      wavPath;
    qint64                       dataBytes = 0;
    qint64                       lastLevelMs = 0;

    // Linear-interpolation resampler state (one sample carry between blocks).
    double                       resamplePos = 0.0;
    double                       lastSample  = 0.0;

    // For converting native frames → mono float, then resampling to 16 kHz.
    QPointer<WasapiVoiceCapture> owner;
};

WasapiVoiceCapture::WasapiVoiceCapture(QObject* parent)
    : IVoiceCapture(parent), d_(std::make_unique<Impl>()) {
    d_->owner = this;
}

WasapiVoiceCapture::~WasapiVoiceCapture() {
    if (recording_.load(std::memory_order_acquire)) {
        cancel();
    }
}

bool WasapiVoiceCapture::start(const QString& outputWavPath) {
    if (recording_.load(std::memory_order_acquire)) {
        cancel();
    }

    // Per-thread COM init (MTA). WASAPI works in either apartment, MTA keeps
    // the worker thread free of message-pump assumptions.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool ownsCom = (hr == S_OK || hr == S_FALSE);

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void**>(&d_->enumerator));
    if (FAILED(hr) || !d_->enumerator.p) {
        emit error(QStringLiteral("WASAPI: enumerator falhou (0x%1)").arg(hr, 8, 16, QChar('0')));
        if (ownsCom) CoUninitialize();
        return false;
    }

    hr = d_->enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &d_->device);
    if (FAILED(hr) || !d_->device.p) {
        emit error(QStringLiteral("WASAPI: nenhum microfone disponível"));
        d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    hr = d_->device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(&d_->client));
    if (FAILED(hr) || !d_->client.p) {
        emit error(QStringLiteral("WASAPI: Activate falhou"));
        d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    hr = d_->client->GetMixFormat(&d_->mixFormat);
    if (FAILED(hr) || !d_->mixFormat) {
        emit error(QStringLiteral("WASAPI: GetMixFormat falhou"));
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    // 100 ms buffer requested; WASAPI may round up but never down.
    const REFERENCE_TIME bufferDuration = 1000000; // 100 ms in 100-ns units
    hr = d_->client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                bufferDuration, 0, d_->mixFormat, nullptr);
    if (FAILED(hr)) {
        emit error(QStringLiteral("WASAPI: Initialize falhou (0x%1)").arg(hr, 8, 16, QChar('0')));
        CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr;
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    HANDLE rawEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!rawEvent) {
        emit error(QStringLiteral("WASAPI: CreateEvent falhou"));
        CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr;
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }
    d_->bufferEvent.reset(rawEvent);

    hr = d_->client->SetEventHandle(d_->bufferEvent.get());
    if (FAILED(hr)) {
        emit error(QStringLiteral("WASAPI: SetEventHandle falhou"));
        d_->bufferEvent.reset();
        CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr;
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    hr = d_->client->GetService(__uuidof(IAudioCaptureClient),
                                reinterpret_cast<void**>(&d_->capture));
    if (FAILED(hr) || !d_->capture.p) {
        emit error(QStringLiteral("WASAPI: GetService(capture) falhou"));
        d_->bufferEvent.reset();
        CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr;
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    // Open WAV file.
    d_->wavPath   = outputWavPath;
    d_->dataBytes = 0;
    d_->lastLevelMs = 0;
    d_->resamplePos = 0.0;
    d_->lastSample  = 0.0;
    level_.store(0.0f, std::memory_order_relaxed);

    d_->file = new QFile(d_->wavPath);
    if (!d_->file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit error(QStringLiteral("Não consegui criar arquivo: %1").arg(d_->wavPath));
        delete d_->file; d_->file = nullptr;
        d_->capture.reset(); d_->bufferEvent.reset();
        CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr;
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }
    d_->file->write(makeWavHeader(kSampleRate, kChannels, kBitsPerSample));

    hr = d_->client->Start();
    if (FAILED(hr)) {
        emit error(QStringLiteral("WASAPI: Start falhou"));
        d_->file->close();
        QFile::remove(d_->wavPath);
        delete d_->file; d_->file = nullptr;
        d_->capture.reset(); d_->bufferEvent.reset();
        CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr;
        d_->client.reset(); d_->device.reset(); d_->enumerator.reset();
        if (ownsCom) CoUninitialize();
        return false;
    }

    d_->shouldRun.store(true, std::memory_order_release);
    recording_.store(true, std::memory_order_release);

    // Worker thread owns the COM apartment + capture loop.
    d_->worker = std::thread([this]() {
        HRESULT thr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool ownsComLocal = (thr == S_OK || thr == S_FALSE);

        // AvSetMmThreadCharacteristicsW("Audio", …) boosts priority so we
        // don't get pre-empted between buffer events. Failure is non-fatal.
        DWORD avTaskIndex = 0;
        HANDLE avTask = AvSetMmThreadCharacteristicsW(L"Audio", &avTaskIndex);

        const WAVEFORMATEX* fmt = d_->mixFormat;
        const UINT32 srcRate    = fmt->nSamplesPerSec;
        const UINT16 srcCh      = fmt->nChannels;
        const UINT16 srcBits    = fmt->wBitsPerSample;
        const bool   srcIsFloat = waveFormatIsFloat(fmt);
        const bool   srcIsPcm   = waveFormatIsPcm(fmt);
        const double rateRatio  = double(srcRate) / double(kSampleRate);

        while (d_->shouldRun.load(std::memory_order_acquire)) {
            DWORD wr = WaitForSingleObject(d_->bufferEvent.get(), 200);
            if (wr == WAIT_TIMEOUT) continue;
            if (!d_->shouldRun.load(std::memory_order_acquire)) break;

            UINT32 packet = 0;
            d_->capture->GetNextPacketSize(&packet);
            while (packet > 0 && d_->shouldRun.load(std::memory_order_acquire)) {
                BYTE*  raw    = nullptr;
                UINT32 frames = 0;
                DWORD  flags  = 0;
                HRESULT gh = d_->capture->GetBuffer(&raw, &frames, &flags, nullptr, nullptr);
                if (FAILED(gh) || !raw || frames == 0) {
                    if (SUCCEEDED(gh)) d_->capture->ReleaseBuffer(frames);
                    d_->capture->GetNextPacketSize(&packet);
                    continue;
                }

                // 1. Downmix native interleaved → mono float in [-1,1].
                std::vector<double> mono(frames);
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    std::fill(mono.begin(), mono.end(), 0.0);
                } else if (srcIsFloat && srcBits == 32) {
                    const float* src = reinterpret_cast<const float*>(raw);
                    for (UINT32 i = 0; i < frames; ++i) {
                        double acc = 0.0;
                        for (UINT16 c = 0; c < srcCh; ++c) acc += double(src[i * srcCh + c]);
                        mono[i] = acc / double(srcCh);
                    }
                } else if (srcIsPcm && srcBits == 16) {
                    const int16_t* src = reinterpret_cast<const int16_t*>(raw);
                    for (UINT32 i = 0; i < frames; ++i) {
                        int32_t acc = 0;
                        for (UINT16 c = 0; c < srcCh; ++c) acc += src[i * srcCh + c];
                        mono[i] = (double(acc) / double(srcCh)) / 32768.0;
                    }
                } else if (srcIsPcm && srcBits == 32) {
                    const int32_t* src = reinterpret_cast<const int32_t*>(raw);
                    for (UINT32 i = 0; i < frames; ++i) {
                        int64_t acc = 0;
                        for (UINT16 c = 0; c < srcCh; ++c) acc += src[i * srcCh + c];
                        mono[i] = (double(acc) / double(srcCh)) / 2147483648.0;
                    }
                } else {
                    // Unknown sub-format — write nothing; release and continue.
                    d_->capture->ReleaseBuffer(frames);
                    d_->capture->GetNextPacketSize(&packet);
                    continue;
                }

                // 2. Linear-interpolation resample → 16 kHz. We keep the last
                // input sample across blocks so the join is continuous.
                std::vector<int16_t> out;
                out.reserve(size_t(double(frames) / rateRatio) + 4);

                double pos = d_->resamplePos;
                double last = d_->lastSample;
                while (pos < double(frames)) {
                    const double idxD = pos;
                    const int    i0   = int(std::floor(idxD));
                    const double frac = idxD - double(i0);
                    const double a = (i0 < 0) ? last : mono[i0];
                    const double b = (i0 + 1 < int(frames)) ? mono[i0 + 1]
                                                            : mono[i0]; // edge clamp
                    const double s = a + (b - a) * frac;
                    const int v = int(std::clamp(s, -1.0, 1.0) * 32767.0);
                    out.push_back(int16_t(v));
                    pos += rateRatio;
                }
                // Carry: shift accumulator back into the next block's frame domain.
                d_->resamplePos = pos - double(frames);
                d_->lastSample  = mono.empty() ? last : mono.back();

                d_->capture->ReleaseBuffer(frames);

                // 3. Write + RMS.
                if (!out.empty() && d_->file) {
                    const qint64 bytes = qint64(out.size()) * sizeof(int16_t);
                    d_->file->write(reinterpret_cast<const char*>(out.data()), bytes);
                    d_->dataBytes += bytes;

                    double sumSq = 0.0;
                    for (int16_t s : out) {
                        const double v = double(s) / 32768.0;
                        sumSq += v * v;
                    }
                    const float rms  = float(std::sqrt(sumSq / double(out.size())));
                    const float norm = std::clamp(std::sqrt(rms) * 1.5f, 0.0f, 1.0f);
                    const float prev = level_.load(std::memory_order_relaxed);
                    const float sm   = prev + (norm - prev) * kLevelAlpha;
                    level_.store(sm, std::memory_order_relaxed);

                    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                    if (nowMs - d_->lastLevelMs >= kLevelMinIntervalMs) {
                        d_->lastLevelMs = nowMs;
                        QMetaObject::invokeMethod(this, [this, sm]() {
                            emit levelChanged(sm);
                        }, Qt::QueuedConnection);
                    }
                }

                d_->capture->GetNextPacketSize(&packet);
            }
        }

        if (avTask) AvRevertMmThreadCharacteristics(avTask);
        if (ownsComLocal) CoUninitialize();
    });

    // The outer CoInitialize was only needed for the synchronous setup calls
    // above — pair it now so we don't leak an apartment on the UI thread.
    if (ownsCom) CoUninitialize();
    return true;
}

void WasapiVoiceCapture::stop() {
    if (!recording_.load(std::memory_order_acquire)) return;

    d_->shouldRun.store(false, std::memory_order_release);
    if (d_->bufferEvent) SetEvent(d_->bufferEvent.get());
    if (d_->worker.joinable()) d_->worker.join();

    if (d_->client.p) d_->client->Stop();
    d_->capture.reset();
    d_->client.reset();
    d_->device.reset();
    d_->enumerator.reset();
    d_->bufferEvent.reset();
    if (d_->mixFormat) { CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr; }

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
    recording_.store(false, std::memory_order_release);
    level_.store(0.0f, std::memory_order_relaxed);
    emit levelChanged(0.0f);
    emit recordingStopped(path);
}

void WasapiVoiceCapture::cancel() {
    const bool wasRecording = recording_.load(std::memory_order_acquire);

    d_->shouldRun.store(false, std::memory_order_release);
    if (d_->bufferEvent) SetEvent(d_->bufferEvent.get());
    if (d_->worker.joinable()) d_->worker.join();

    if (d_->client.p) d_->client->Stop();
    d_->capture.reset();
    d_->client.reset();
    d_->device.reset();
    d_->enumerator.reset();
    d_->bufferEvent.reset();
    if (d_->mixFormat) { CoTaskMemFree(d_->mixFormat); d_->mixFormat = nullptr; }

    if (d_->file) {
        d_->file->close();
        delete d_->file; d_->file = nullptr;
    }
    if (!d_->wavPath.isEmpty()) {
        QFile::remove(d_->wavPath);
        d_->wavPath.clear();
    }
    recording_.store(false, std::memory_order_release);
    if (wasRecording) {
        level_.store(0.0f, std::memory_order_relaxed);
        emit levelChanged(0.0f);
    }
}

} // namespace dante::audio

// =====================================================================
// Non-Windows stub — keep the TU compilable on macOS/Linux so CMake doesn't
// need a platform guard around the source list entry.
// =====================================================================
#else

namespace dante::audio {

struct WasapiVoiceCapture::Impl {};

WasapiVoiceCapture::WasapiVoiceCapture(QObject* parent)
    : IVoiceCapture(parent), d_(std::make_unique<Impl>()) {}
WasapiVoiceCapture::~WasapiVoiceCapture() = default;

bool WasapiVoiceCapture::start(const QString& /*outputWavPath*/) {
    emit error(QStringLiteral("WASAPI só está disponível no Windows."));
    return false;
}
void WasapiVoiceCapture::stop()   {}
void WasapiVoiceCapture::cancel() {}

} // namespace dante::audio

#endif // _WIN32
