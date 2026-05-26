// macOS-native AVFoundation microphone capture.
//
// STATUS: stub. We default to QtVoiceCapture (Qt Multimedia, which itself
// sits on AVAudioEngine on macOS) — it already gives us mono s16 16 kHz and
// triggers the system microphone-permission prompt the first time we call
// QAudioSource::start(). A bespoke AVFoundation path would mainly buy us:
//   • Finer control over the AVAudioSession category (mixWithOthers, etc).
//   • Direct access to the input meter levels (no manual RMS pass).
//   • A future "voice activity detection" hook (silence → auto-stop).
// File extension is .h+.cpp (not .mm) because the stub avoids Objective-C
// symbols entirely. When implemented, rename to .mm and add AVFoundation
// frameworks to the CMake target.
#pragma once

#include "IVoiceCapture.h"
#include <QString>

namespace dante::audio {

class MacVoiceCapture : public IVoiceCapture {
    Q_OBJECT
public:
    explicit MacVoiceCapture(QObject* parent = nullptr);
    ~MacVoiceCapture() override;

    bool  start(const QString& outputWavPath) override;
    void  stop() override;
    void  cancel() override;
    float currentLevel() const override { return 0.0f; }
    bool  isRecording() const override  { return false; }
};

} // namespace dante::audio
