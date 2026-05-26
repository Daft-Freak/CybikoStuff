cmake_minimum_required(VERSION 3.13)

project(LCDTest C CXX ASM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(../../lib lib)

add_executable(LCDTest
    Main.cpp
)

target_link_libraries(LCDTest cybiko_crt0 H8)

make_bootable(LCDTest)
