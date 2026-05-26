// Windows-native WASAPI shared-mode microphone capture.
//
// STATUS: stub. Production builds currently use QtVoiceCapture (Qt
// Multimedia) on both Windows and macOS — see VoiceController for the
// factory choice. This file is here so the Windows-specific implementation
// can be slotted in later without touching the controller or the QML layer
// (both program against IVoiceCapture).
//
// Planned implementation outline (when we want the lower-latency path):
//   • CoInitializeEx(COINIT_MULTITHREADED)
//   • IMMDeviceEnumerator → GetDefaultAudioEndpoint(eCapture, eConsole)
//   • IAudioClient::Initialize(AUDCLNT_SHAREMODE_SHARED, …, &mixFormat)
//   • IAudioCaptureClient::GetBuffer in a dedicated worker thread
//   • Resample mixFormat → 16 kHz mono s16 (libsamplerate or hand-rolled
//     linear if the device is already at 48 kHz — common case)
//   • Reuse QtVoiceCapture's WAV-header writer (makeWavHeader is static)
#pragma once

#include "IVoiceCapture.h"
#include <QString>

namespace dante::audio {

class WasapiVoiceCapture : public IVoiceCapture {
    Q_OBJECT
public:
    explicit WasapiVoiceCapture(QObject* parent = nullptr);
    ~WasapiVoiceCapture() override;

    bool  start(const QString& outputWavPath) override;
    void  stop() override;
    void  cancel() override;
    float currentLevel() const override { return 0.0f; }
    bool  isRecording() const override  { return false; }
};

} // namespace dante::audio
