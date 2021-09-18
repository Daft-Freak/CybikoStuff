# H8 Peripherals
add_library(H8 INTERFACE)

target_sources(H8 INTERFACE
    Serial.cpp
)

target_include_directories(H8 INTERFACE ${CMAKE_CURRENT_LIST_DIR})
