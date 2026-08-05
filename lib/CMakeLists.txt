cmake_minimum_required(VERSION 3.13)

project(cybiko)

if(NOT DEFINED LIB_ONCE)
	set(LIB_ONCE TRUE)

    add_subdirectory(H8)


    function(make_bootable TARGET)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}>.boot
        )

        if(CYBIKO_XTREME)
            add_custom_target(${TARGET}.boot DEPENDS ${TARGET} COMMAND ${TOOLS_PATH}/usbcon -b ${TARGET}.boot)
        elseif(CYBIKO_SERIAL_PORT)
            add_custom_target(${TARGET}.boot DEPENDS ${TARGET} COMMAND ${TOOLS_PATH}/sercon -p ${CYBIKO_SERIAL_PORT} -b ${TARGET}.boot)
        endif()
    endfunction()
endif()
