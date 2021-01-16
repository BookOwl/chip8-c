cmake_minimum_required(VERSION 3.17)
project(chip8_c C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)

set(SDL2_PATH "C:\\C-Libs\\SDL2-2.0.14")
find_package(SDL2 REQUIRED)
include_directories( ${SDL2_INCLUDE_DIR} )

add_executable(chip8_c main.c)
target_link_libraries(chip8_c ${SDL2_LIBRARY})
message("SDL2: ${SDL2_LIBRARY}")