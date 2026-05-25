cmake_minimum_required(VERSION 3.10)

project(CybikoTools)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# shared encoding code
add_library(encoding STATIC
    crc.c
    encoding.c
)

# archive tool
set(ARCHIVE_SOURCES
    archive.c
)

add_executable(archive ${ARCHIVE_SOURCES})
target_link_libraries(archive encoding)

# flash image extractor
set(FLASHSPLIT_SOURCES
    flashsplit.c
)

add_executable(flashsplit ${FLASHSPLIT_SOURCES})
target_link_libraries(flashsplit encoding)

# boot image encode/decode tool
set(IMGTOOL_SOURCES
    imgtool.c
)

add_executable(imgtool ${IMGTOOL_SOURCES})
target_link_libraries(imgtool encoding)


# usb boot/console tool
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PKG_LIBUSB libusb-1.0)
endif()

if(PKG_LIBUSB_FOUND)
    message("Found libusb, building usbcon")
    set(USBCON_SOURCES
        usbcon.c
    )

    add_executable(usbcon ${USBCON_SOURCES})
    target_include_directories(usbcon PRIVATE ${PKG_LIBUSB_INCLUDE_DIRS})
    target_link_libraries(usbcon encoding ${PKG_LIBUSB_LIBRARIES})
endif()

# serial boot/console tool
find_library(LIBSERIALPORT_LIBRARIES serialport)
find_path(LIBSERIALPORT_INCLUDE_DIR libserialport.h)

if(LIBSERIALPORT_LIBRARIES AND LIBSERIALPORT_INCLUDE_DIR)
    message("Found libserialport, building sercon")
    set(SERCON_SOURCES
        sercon.c
    )

    add_executable(sercon ${SERCON_SOURCES})
    target_include_directories(sercon PRIVATE ${LIBSERIALPORT_INCLUDE_DIR})
    target_link_libraries(sercon encoding ${LIBSERIALPORT_LIBRARIES})
endif()
