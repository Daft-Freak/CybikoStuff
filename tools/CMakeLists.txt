cmake_minimum_required(VERSION 3.10)

project(CybikoTools)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fsanitize=address")

set(IMGTOOL_SOURCES
    encoding.c
    imgtool.c
)

add_executable(imgtool ${IMGTOOL_SOURCES})

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PKG_LIBUSB libusb-1.0)
endif()

if(PKG_LIBUSB_FOUND)
    set(USBCON_SOURCES
        encoding.c
        usbcon.c
    )

    add_executable(usbcon ${USBCON_SOURCES})
    target_include_directories(usbcon PRIVATE ${PKG_LIBUSB_INCLUDE_DIRS})
    target_link_libraries(usbcon ${PKG_LIBUSB_LIBRARIES})
endif()
