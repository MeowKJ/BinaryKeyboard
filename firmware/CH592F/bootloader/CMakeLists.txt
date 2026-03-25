cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/toolchain-ch59x.cmake)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CH592F_IAP C ASM)

set(TARGET_ELF ${PROJECT_NAME}.elf)

set(BOOTLOADER_SOURCES
    main.c
    ../SDK/Startup/startup_CH592.S
    ../SDK/StdPeriphDriver/CH59x_sys.c
    ../SDK/StdPeriphDriver/CH59x_clk.c
)

add_executable(${TARGET_ELF} ${BOOTLOADER_SOURCES})

target_include_directories(${TARGET_ELF} PRIVATE
    ../SDK/StdPeriphDriver/inc
    ../SDK/RVMSIS
    ../iap
)

target_compile_definitions(${TARGET_ELF} PRIVATE
    FREQ_SYS=60000000
)

target_compile_options(${TARGET_ELF} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=gnu99 -Wall -Os>
)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/Ld/Link.ld)

target_link_options(${TARGET_ELF} PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
)

target_link_libraries(${TARGET_ELF} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDK/StdPeriphDriver/libISP592.a
)

# Post-build: generate .bin and .hex
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET_ELF}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin"
    VERBATIM)

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJCOPY} -Oihex $<TARGET_FILE:${TARGET_ELF}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMENT "Generating ${PROJECT_NAME}.hex"
    VERBATIM)

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_SIZE} --format=berkeley $<TARGET_FILE:${TARGET_ELF}>
    COMMENT "IAP size:"
    VERBATIM)
