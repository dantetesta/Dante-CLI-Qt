#include "WasapiVoiceCapture.h"

#include <QFile>

namespace dante::audio {

WasapiVoiceCapture::WasapiVoiceCapture(QObject* parent)
    : IVoiceCapture(parent) {}

WasapiVoiceCapture::~WasapiVoiceCapture() = default;

#if defined(Q_OS_WIN)

// TODO(audio): real WASAPI capture. See header for the planned outline.
// For now, emit an error so callers can fall back to QtVoiceCapture.

bool WasapiVoiceCapture::start(const QString& /*outputWavPath*/) {
    emit error("WASAPI capture não implementado nesta build — use QtVoiceCapture.");
    return false;
}

void WasapiVoiceCapture::stop()   {}
void WasapiVoiceCapture::cancel() {}

#else

// Non-Windows stub: building this file on macOS/Linux is harmless (no
// symbols pulled in) but it must compile clean.
bool WasapiVoiceCapture::start(const QString& /*outputWavPath*/) {
    emit error("WASAPI só está disponível no Windows.");
    return false;
}
void WasapiVoiceCapture::stop()   {}
void WasapiVoiceCapture::cancel() {}

#endif

} // namespace dante::audio
