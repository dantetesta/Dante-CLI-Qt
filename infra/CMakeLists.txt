# Infra layer — adapters to OS, processes, network transports.
add_library(dante-infra STATIC
    shell-adapters/ShellAdapter.cpp
    shell-adapters/ShellAdapter.h
    shell-adapters/ConPtyAdapter.cpp
    shell-adapters/ConPtyAdapter.h
    shell-adapters/UnixPtyAdapter.cpp
    shell-adapters/UnixPtyAdapter.h
    audio-adapters/IVoiceCapture.h
    audio-adapters/QtVoiceCapture.cpp
    audio-adapters/QtVoiceCapture.h
    audio-adapters/WasapiVoiceCapture.cpp
    audio-adapters/WasapiVoiceCapture.h
    audio-adapters/MacVoiceCapture.cpp
    audio-adapters/MacVoiceCapture.h
)
target_include_directories(dante-infra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dante-infra PUBLIC
    Qt6::Core Qt6::Multimedia dante-core
    $<$<PLATFORM_ID:Linux>:util>
)
set_target_properties(dante-infra PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)

# SPEC-150 — AVFoundation native capture on macOS. Compile the file as
# Objective-C++ so #import <AVFoundation/...> and Obj-C blocks resolve.
if(APPLE)
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

# SPEC-151 — WASAPI capture on Windows.
if(WIN32)
    target_link_libraries(dante-infra PRIVATE ole32 oleaut32 uuid winmm avrt)
endif()
