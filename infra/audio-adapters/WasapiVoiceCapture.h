// Windows-native WASAPI shared-mode microphone capture.
//
// Capture pipeline:
//   1. CoInitializeEx (MTA) on the worker thread.
//   2. IMMDeviceEnumerator → GetDefaultAudioEndpoint(eCapture, eCommunications)
//      — eCommunications gives the OS-preferred mic for voice apps and
//      participates in the system's mic-mute toggles.
//   3. IAudioClient::Initialize(SHARED, EVENTCALLBACK, …). Buffer event is
//      signaled by WASAPI whenever new audio is available.
//   4. Worker thread WaitForSingleObject(event, …) → IAudioCaptureClient::
//      GetBuffer → resample device-native format → 16 kHz mono Int16 →
//      append to WAV file.
//   5. Stop()/cancel() flip an atomic, signal the event, join the worker,
//      finalize/discard the WAV. Cleanup is RAII-released.
//
// Output WAV matches QtVoiceCapture / MacVoiceCapture exactly: PCM s16 LE,
// mono, 16 kHz, 44-byte canonical RIFF header.
#pragma once

#include "IVoiceCapture.h"
#include <QString>

#include <atomic>
#include <memory>

namespace dante::audio {

class WasapiVoiceCapture : public IVoiceCapture {
    Q_OBJECT
public:
    explicit WasapiVoiceCapture(QObject* parent = nullptr);
    ~WasapiVoiceCapture() override;

    bool  start(const QString& outputWavPath) override;
    void  stop() override;
    void  cancel() override;
    float currentLevel() const override { return level_.load(std::memory_order_relaxed); }
    bool  isRecording() const override  { return recording_.load(std::memory_order_acquire); }

private:
    struct Impl;
    std::unique_ptr<Impl> d_;

    std::atomic<bool>  recording_{false};
    std::atomic<float> level_{0.0f};
};

} // namespace dante::audio
