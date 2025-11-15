cmake_minimum_required(VERSION 3.20)
project(cretris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

add_executable(cretris
    src/core/Game.cpp
    src/core/Tetromino.cpp
    src/frontend/ncurses/NcursesFrontend.cpp
    src/main.cpp)

target_include_directories(cretris PRIVATE src)

target_link_libraries(cretris PRIVATE ${CURSES_LIBRARIES})

target_compile_options(cretris PRIVATE -Wall -Wextra -pedantic)

