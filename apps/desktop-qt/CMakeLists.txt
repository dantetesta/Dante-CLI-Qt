# Desktop Qt app — links core + infra, registers QML types, exposes models.

qt_add_executable(dante-cli WIN32
    src/main.cpp
    src/AppState.cpp
    src/AppState.h
    src/TabsModel.cpp
    src/TabsModel.h
    src/FavoritesModel.cpp
    src/FavoritesModel.h
    src/SnippetsModel.cpp
    src/SnippetsModel.h
    src/CredentialsModel.cpp
    src/CredentialsModel.h
    src/TerminalBridge.cpp
    src/TerminalBridge.h
)

# QML files live outside the target's source dir (../../ui/qml/). Without an
# explicit QT_RESOURCE_ALIAS each file's resource path inherits the physical
# location ("ui/qml/Main.qml"), which makes loadFromModule("dante.ui","Main")
# fail at runtime. Aliasing each file to its bare name puts it under the
# module URI as expected.
foreach(qml_file Main TabBar TabChip Sidebar Terminal BottomToolbar Theme)
    set_source_files_properties(
        ../../ui/qml/${qml_file}.qml
        PROPERTIES QT_RESOURCE_ALIAS ${qml_file}.qml
    )
endforeach()

# Theme.qml is a singleton (pragma Singleton).
set_source_files_properties(../../ui/qml/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(dante-cli
    URI dante.ui
    VERSION 1.0
    QML_FILES
        ../../ui/qml/Main.qml
        ../../ui/qml/TabBar.qml
        ../../ui/qml/TabChip.qml
        ../../ui/qml/Sidebar.qml
        ../../ui/qml/Terminal.qml
        ../../ui/qml/BottomToolbar.qml
        ../../ui/qml/Theme.qml
)

target_link_libraries(dante-cli PRIVATE
    dante-core
    dante-infra
    Qt6::Quick Qt6::QuickControls2 Qt6::Widgets
)

set_target_properties(dante-cli PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    OUTPUT_NAME "Dante CLI"
)

# Windows: copy required Qt DLLs next to the .exe so the build folder is
# ready to ship. Production installer (Inno Setup) bundles the same set.
if(WIN32)
    add_custom_command(TARGET dante-cli POST_BUILD
        COMMAND Qt6::windeployqt
            --qmldir "${CMAKE_SOURCE_DIR}/ui/qml"
            "$<TARGET_FILE:dante-cli>"
        COMMENT "Running windeployqt"
    )
endif()
