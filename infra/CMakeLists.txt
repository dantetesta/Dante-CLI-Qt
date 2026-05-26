# Infra layer — adapters to OS, processes, network transports.
add_library(dante-infra STATIC
    shell-adapters/ShellAdapter.cpp
    shell-adapters/ShellAdapter.h
)
target_include_directories(dante-infra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dante-infra PUBLIC Qt6::Core dante-core)
set_target_properties(dante-infra PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
