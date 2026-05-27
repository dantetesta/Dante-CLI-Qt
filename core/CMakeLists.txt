# Core domain — pure C++ that knows nothing about UI.
add_library(dante-core STATIC
    persistence/JsonStore.cpp
    persistence/JsonStore.h
    persistence/AppPaths.cpp
    persistence/AppPaths.h
    calculator/CalculatorEngine.cpp
    calculator/CalculatorEngine.h
    terminal/TerminalSession.cpp
    terminal/TerminalSession.h
    terminal/TerminalRegistry.cpp
    terminal/TerminalRegistry.h
    terminal/IPtyAdapter.h
    terminal/VTParser.cpp
    terminal/VTParser.h
    terminal/TerminalBuffer.cpp
    terminal/TerminalBuffer.h
    themes/TerminalScheme.cpp
    themes/TerminalScheme.h
    themes/ThemeRegistry.cpp
    themes/ThemeRegistry.h
    ai/GroqClient.cpp
    ai/GroqClient.h
    ai/AIProvider.h
    ai/AIProviderStore.cpp
    ai/AIProviderStore.h
    update/UpdateChecker.cpp
    update/UpdateChecker.h
    domain/Models.h
    domain/EventBus.cpp
    domain/EventBus.h
    claude/ClaudeResources.h
    claude/ClaudeResourceScanner.cpp
    claude/ClaudeResourceScanner.h
    autofill/AutoFillEngine.cpp
    autofill/AutoFillEngine.h
    generators/Generator.h
    generators/GeneratorsRegistry.cpp
    generators/GeneratorsRegistry.h
    telemetry/Logger.cpp
    telemetry/Logger.h
    system/ProcessStats.cpp
    system/ProcessStats.h
)

target_include_directories(dante-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dante-core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Network Qt6::Sql Qt6::Concurrent
)

if(WIN32)
    target_link_libraries(dante-core PUBLIC psapi)
endif()

set_target_properties(dante-core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)
