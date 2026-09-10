cmake_minimum_required(VERSION 3.19)
project(NetworkServer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(NetworkServer
    WIN32 MACOSX_BUNDLE
    main.cpp
    server_window.cpp
    server_window.h
    server_worker.h server_worker.cpp
    config_dialog.h config_dialog.cpp
)

# Добавляем src/ в пути поиска заголовков для простого включения protocol.h
target_include_directories(NetworkServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(NetworkServer
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)

install(TARGETS NetworkServer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET NetworkServer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
