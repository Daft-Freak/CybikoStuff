cmake_minimum_required(VERSION 3.9)

project(CybikoEmu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(usbip-server)

set(SOURCES
    DS2401.cpp
    H8CPU.cpp
    KeyboardDevice.cpp
    LCDDevice.cpp
    Main.cpp
    MemoryDevice.cpp
    PCF8593.cpp
    SerialFlash.cpp
    USBDevice.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_executable(emu ${SOURCES})

find_package(SDL2 REQUIRED)
target_link_libraries(emu usbip-server SDL2::SDL2)

set_target_properties(emu PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

