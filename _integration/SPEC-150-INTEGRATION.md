# SPEC-150 — macOS AVFoundation mic capture integration

The new `MacVoiceCapture` implementation is a real AVFoundation pipeline
(`AVAudioEngine` + `AVAudioConverter` → 16 kHz mono Int16 LE RIFF/WAVE,
identical to what `QtVoiceCapture` produces, so the rest of the GroqWhisper
upload path needs no changes). The file is **still named `.cpp`** — the
Objective-C++ section is gated behind `#ifdef __APPLE__`. To compile it the
build system needs to be told to treat it as Objective-C++ on APPLE; see
the CMake patch below.

## `infra/CMakeLists.txt`

Two changes on APPLE only — language hint plus framework linkage. Append
**after** the existing `add_library(dante-infra STATIC … MacVoiceCapture.cpp …)`
block:

```cmake
if(APPLE)
    # Force OBJCXX so the #import <AVFoundation/AVFoundation.h> and Obj-C
    # blocks inside MacVoiceCapture.cpp compile. The file is otherwise
    # plain C++ on non-Apple targets.
    set_source_files_properties(
        audio-adapters/MacVoiceCapture.cpp
        PROPERTIES LANGUAGE OBJCXX
    )
    target_link_libraries(dante-infra PRIVATE
        "-framework AVFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework Foundation"
    )
endif()
```

(Equivalent alternative: rename the file to `MacVoiceCapture.mm` and update
the source-list entry. The `#ifdef __APPLE__` guards in the file already
keep the non-Apple TU a no-op, so either approach works — the patch above
avoids touching anything cross-platform.)

No other source-list change is required. The CMakeLists already lists
`audio-adapters/MacVoiceCapture.cpp` and `MacVoiceCapture.h`.

## Info.plist (macOS bundle)

`AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio` requires the
bundle to declare a microphone-usage string, or the OS will reject the
prompt with `TCC violation` and the app will terminate.

**There is currently no `Info.plist` template under `apps/desktop-qt/`.**
The human needs to either:

1. Create `apps/desktop-qt/Info.plist.in` containing at minimum:

   ```xml
   <?xml version="1.0" encoding="UTF-8"?>
   <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
   <plist version="1.0">
   <dict>
       <key>CFBundleExecutable</key>
       <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
       <key>CFBundleIdentifier</key>
       <string>${MACOSX_BUNDLE_GUI_IDENTIFIER}</string>
       <key>NSMicrophoneUsageDescription</key>
       <string>Dante CLI usa o microfone para transcrever comandos de voz.</string>
   </dict>
   </plist>
   ```

   and point the executable at it via
   `set_target_properties(dante-cli PROPERTIES MACOSX_BUNDLE_INFO_PLIST
   "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")`; or

2. Add the key directly to the existing generated plist post-build (less
   portable; not recommended).

Without this key the very first `start()` call will surface the
`Permissão de microfone negada` error from MacVoiceCapture even before
the user sees any system prompt.

## `apps/desktop-qt/src/VoiceController.cpp`

The current factory hard-wires `QtVoiceCapture` for both platforms. To
exercise the new AVFoundation path on macOS, change `makeCapture` to
something like:

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

Include `audio-adapters/MacVoiceCapture.h` and
`audio-adapters/WasapiVoiceCapture.h` at the top alongside the existing
`QtVoiceCapture.h`. No other consumer change is needed — the new class
honours `IVoiceCapture` verbatim.

## Verification

- macOS: launch → toolbar mic button → grant permission on first run →
  speak → release. The waveform indicator should respond live (proves
  the RMS path is active), and `recordingStopped(path)` should produce a
  valid 16 kHz mono s16 LE WAV the existing Whisper upload accepts.
- Sanity check the wav: `afinfo /tmp/dante-voice-*.wav` → should report
  "Linear PCM, 16-bit signed integer, 16000 Hz, 1 channel".
- Cancel path: start, then cancel without stop → file must not exist
  afterwards (the destructor + `cancel()` both call `QFile::remove`).
- Permission denied path: revoke mic in System Settings → Privacy →
  Microphone → next `start()` must emit `error("Permissão de microfone
  negada")` and return false (no leaked engine, no orphan wav file).
