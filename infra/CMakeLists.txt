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
