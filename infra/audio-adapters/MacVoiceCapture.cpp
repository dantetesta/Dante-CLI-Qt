#include "MacVoiceCapture.h"

namespace dante::audio {

MacVoiceCapture::MacVoiceCapture(QObject* parent)
    : IVoiceCapture(parent) {}

MacVoiceCapture::~MacVoiceCapture() = default;

// TODO(audio): real AVFoundation capture (AVAudioRecorder, mirroring Swift
// `AudioRecorder.swift`). See header for the planned scope. For now this
// stub just errors out so the factory in VoiceController can fall through.

bool MacVoiceCapture::start(const QString& /*outputWavPath*/) {
    emit error("AVFoundation capture não implementado — use QtVoiceCapture.");
    return false;
}

void MacVoiceCapture::stop()   {}
void MacVoiceCapture::cancel() {}

} // namespace dante::audio
