cmake_minimum_required(VERSION 3.19)
project(NetworkClient LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable(NetworkClient
    main.cpp
    network_client.h network_client.cpp
    data_generator.h data_generator.cpp
)

# Добавляем src/ в пути поиска заголовков для простого включения protocol.h
target_include_directories(NetworkClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(NetworkClient
    PRIVATE
        Qt6::Core
        Qt6::Network
)

install(TARGETS NetworkClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
