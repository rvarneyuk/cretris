cmake_minimum_required(VERSION 3.20)
project(cretris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)
find_package(SDL2 QUIET)
if (NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})
endif()

add_executable(cretris
    src/core/Game.cpp
    src/core/Tetromino.cpp
    src/frontend/ncurses/NcursesFrontend.cpp
    src/frontend/sdl/AudioEngine.cpp
    src/frontend/sdl/SdlFrontend.cpp
    src/main.cpp)

target_include_directories(cretris PRIVATE src)

target_link_libraries(cretris PRIVATE SDL2::SDL2 ${CURSES_LIBRARIES})
if (TARGET SDL2::SDL2main)
    target_link_libraries(cretris PRIVATE SDL2::SDL2main)
endif()

target_compile_options(cretris PRIVATE -Wall -Wextra -pedantic)

