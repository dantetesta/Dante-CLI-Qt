# SPEC-151 — Windows WASAPI mic capture integration

`WasapiVoiceCapture` is now a working WASAPI shared-mode capture client:
default `eCommunications` mic, event-driven worker thread, device-native
format downmixed to mono and linearly resampled to 16 kHz Int16 LE, written
directly into a RIFF/WAVE file matching `QtVoiceCapture` / `MacVoiceCapture`.
The whole Windows section is gated behind `#ifdef _WIN32`; the file
compiles to a no-op stub on macOS/Linux, so no CMake guards are needed on
the source-list entry.

## `infra/CMakeLists.txt`

Append after the existing `target_link_libraries(dante-infra PUBLIC …)`:

```cmake
if(WIN32)
    target_link_libraries(dante-infra PRIVATE
        ole32        # CoCreateInstance / CoTaskMemFree / CoInitializeEx
        oleaut32     # WAVEFORMATEX helpers (also pulled in by mmdevapi headers)
        uuid         # __uuidof(...) for IMMDevice / IAudioClient
        winmm        # Multimedia base
        avrt         # AvSetMmThreadCharacteristics (audio thread priority)
    )
endif()
```

(`mmdevapi.lib` is not a separate import library in modern Windows SDKs —
`ole32` covers everything the WASAPI headers reference.)

No source-list change required: `audio-adapters/WasapiVoiceCapture.cpp` and
`.h` are already listed. The file compiles cross-platform because of the
`#ifdef _WIN32` gate.

## `apps/desktop-qt/src/VoiceController.cpp`

Same factory edit as documented in SPEC-150-INTEGRATION:

```cpp
audio::IVoiceCapture* VoiceController::makeCapture(QObject* parent) {
#if defined(Q_OS_MACOS)
    return new audio::MacVoiceCapture(parent);
#elif defined(Q_OS_WIN)
    return new audio::WasapiVoiceCapture(parent);
#else
    return new audio::QtVoiceCapture(parent);
#endif
}
```

Make sure `audio-adapters/WasapiVoiceCapture.h` is `#include`d at the top.

## Permissions (Windows 10 1903+)

Microphone access is governed by Settings → Privacy → Microphone. Unlike
macOS there is no prompt issued automatically by the API — if the user
has denied per-app or global access, `IAudioClient::Initialize` succeeds
but `GetBuffer` will return `AUDCLNT_E_DEVICE_INVALIDATED` or silent
buffers. We surface this via the existing `error()` signal path; the
human-visible permission grant lives in the Settings app and cannot be
triggered from code.

No appx manifest capability is needed for a classic Win32 build — only
UWP/Store packages require `<DeviceCapability Name="microphone"/>`.

## Verification

- Windows: launch → mic toolbar button → speak → release. Confirm via
  log/UI that:
  - `levelChanged` fires (waveform animates → meter > 0).
  - `recordingStopped(path)` emits a file > 44 bytes (header + samples).
  - The file plays back at the right speed in a media player
    (validates the linear-resampler ratio for the device's native rate
    — usually 48 kHz × 1 ch on consumer headsets, 44.1 kHz × 2 ch on
    USB mics).
- Cancel path: cancel before stop → file must not exist on disk.
- Privacy-disabled path: deny mic access in Windows Settings → next
  `start()` should still succeed but the captured buffer will be silent;
  the user gets blank transcripts and the UI meter stays flat. (A nicer
  future enhancement would call `Windows::Devices::Enumeration::DeviceAccessInformation`
  to surface this proactively; out of scope for v1.)
- Unplug the default capture device mid-recording: the loop should not
  crash — `GetBuffer` returns an error, we release and continue waiting
  on the event; on next `stop()`/`cancel()` everything tears down cleanly.
