if(NOT DEFINED LIB_ONCE)
	set(LIB_ONCE TRUE)

    add_subdirectory(H8)


    function(make_bootable TARGET)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}>.boot
        )

        add_custom_target(${TARGET}.boot DEPENDS ${TARGET} COMMAND ${TOOLS_PATH}/usbcon -b ${TARGET}.boot)
    endfunction()
endif()
