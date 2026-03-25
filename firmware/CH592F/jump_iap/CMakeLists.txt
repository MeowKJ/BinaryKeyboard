cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/toolchain-ch59x.cmake)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CH592F_JumpIAP C ASM)

set(TARGET_ELF ${PROJECT_NAME}.elf)

add_executable(${TARGET_ELF}
    Startup/startup_CH592.S
)

set_target_properties(${TARGET_ELF} PROPERTIES LINKER_LANGUAGE C)

target_link_options(${TARGET_ELF} PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/Ld/Link.ld
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
)

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
    COMMENT "JumpIAP size:"
    VERBATIM)
