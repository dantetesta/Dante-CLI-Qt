// macOS-native AVFoundation microphone capture.
//
// Implementation lives in MacVoiceCapture.cpp, but the entire macOS path is
// gated behind `#ifdef __APPLE__` and uses Objective-C++ constructs. For the
// build to pick that up the source-file LANGUAGE must be set to OBJCXX on
// APPLE (see _integration/SPEC-150-INTEGRATION.md). On non-Apple targets the
// same file builds as a no-op TU.
//
// Capture pipeline (parity with Swift sibling AudioRecorder.swift):
//   1. Request mic permission via AVCaptureDevice requestAccessForMediaType.
//   2. Spin AVAudioEngine, attach a tap on the inputNode.
//   3. Convert each tapped AVAudioPCMBuffer (native float32 @ device rate)
//      → 16 kHz mono Int16 LE via AVAudioConverter.
//   4. Stream the resampled bytes into a RIFF/WAVE file matching the
//      QtVoiceCapture format so the rest of the pipeline (GroqWhisper) is
//      unchanged.
//   5. Compute a smoothed RMS for the waveform UI (levelChanged signal).
#pragma once

#include "IVoiceCapture.h"
#include <QString>

#include <atomic>
#include <memory>

namespace dante::audio {

class MacVoiceCapture : public IVoiceCapture {
    Q_OBJECT
public:
    explicit MacVoiceCapture(QObject* parent = nullptr);
    ~MacVoiceCapture() override;

    bool  start(const QString& outputWavPath) override;
    void  stop() override;
    void  cancel() override;
    float currentLevel() const override { return level_.load(std::memory_order_relaxed); }
    bool  isRecording() const override  { return recording_.load(std::memory_order_acquire); }

private:
    // Opaque PIMPL — definition is Objective-C++ inside the .cpp.
    struct Impl;
    std::unique_ptr<Impl> d_;

    std::atomic<bool>  recording_{false};
    std::atomic<float> level_{0.0f};
};

} // namespace dante::audio
