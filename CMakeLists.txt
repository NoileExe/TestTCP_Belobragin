cmake_minimum_required(VERSION 3.19)
project(TestTCP_Belobragin LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Подключаем подпроекты
add_subdirectory(src/client)
add_subdirectory(src/server)