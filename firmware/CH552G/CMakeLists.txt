cmake_minimum_required(VERSION 3.21)

# Must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-ch552.cmake)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CH552G C)

set(TARGET_IHX ${PROJECT_NAME}.ihx)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(BK_VERSION_HEADER ${GENERATED_DIR}/bk_version_config.h)

add_custom_command(
    OUTPUT ${BK_VERSION_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/scripts/versioning.py
            emit-c-header --chip CH552G --out ${BK_VERSION_HEADER}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/scripts/versioning.py
        ${CMAKE_CURRENT_SOURCE_DIR}/../../config/versions.json
    COMMENT "Generating CH552G version header"
)

add_custom_target(ch552g_version_header DEPENDS ${BK_VERSION_HEADER})

# ---------------------------------------------------------------------------
# Keyboard selection
# ---------------------------------------------------------------------------
set(KEYBOARD "BASIC" CACHE STRING "Keyboard: BASIC, KNOB, 5KEY")
set_property(CACHE KEYBOARD PROPERTY STRINGS "BASIC" "KNOB" "5KEY")
string(TOUPPER "${KEYBOARD}" KEYBOARD_UPPER)

if(KEYBOARD_UPPER STREQUAL "5KEY")
    set(KEYBOARD_DEFINE "USE_5KEYS")
elseif(KEYBOARD_UPPER STREQUAL "KNOB")
    set(KEYBOARD_DEFINE "USE_KNOB")
elseif(KEYBOARD_UPPER STREQUAL "BASIC")
    set(KEYBOARD_DEFINE "USE_BASIC")
else()
    message(FATAL_ERROR "Unsupported KEYBOARD='${KEYBOARD}'. Expected BASIC, KNOB or 5KEY.")
endif()

message(STATUS "Keyboard: ${KEYBOARD_UPPER}")

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES
    # Entry point & Arduino runtime
    SDK/cores/main.c
    User/Main.c

    # Firmware application
    src/CustomUSBHID.c
    src/EncoderHandler.c
    src/KeyScanner.c
    src/KeysDataHandler.c
    src/LightingController.c
    src/USBConstant.c
    src/USBHandler.c
    src/rgb.c

    # ch55xduino SDK — core runtime
    SDK/cores/wiring.c
    SDK/cores/wiring_digital.c
    SDK/cores/eeprom.c

    # ch55xduino SDK — WS2812 LED driver (P1.5 only)
    SDK/libraries/WS2812/template/optionalLink_WS2812_P1_5.c
)

add_executable(${TARGET_IHX} ${SOURCES})
add_dependencies(${TARGET_IHX} ch552g_version_header)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(${TARGET_IHX} PRIVATE
    ${GENERATED_DIR}
    # ch55xduino SDK headers
    SDK/cores                       # Arduino.h, usbCommonDescriptors/, wiring_*.h
    SDK/variants/ch552              # include/ch5xx.h, pins_arduino*.h
    SDK/libraries/WS2812            # WS2812.h
    SDK/libraries/WS2812/template   # WS2812_DELAY.h, WS2812_pins_header.h
    # Firmware headers
    User
    src
)

# ---------------------------------------------------------------------------
# Compiler definitions
# ---------------------------------------------------------------------------
target_compile_definitions(${TARGET_IHX} PRIVATE
    CH552=1
    F_CPU=24000000
    USER_USB_RAM=${CH552_USB_RAM}
    ${KEYBOARD_DEFINE}
)

# ---------------------------------------------------------------------------
# Compiler options
# ---------------------------------------------------------------------------
target_compile_options(${TARGET_IHX} PRIVATE
    --less-pedantic
)

# ---------------------------------------------------------------------------
# Post-build: generate .hex and .bin, print size
# ---------------------------------------------------------------------------
add_custom_command(TARGET ${TARGET_IHX} POST_BUILD
    COMMAND ${SDCC_PACKIHX} $<TARGET_FILE:${TARGET_IHX}>
            > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMENT "Generating ${PROJECT_NAME}.hex (packihx)")

add_custom_command(TARGET ${TARGET_IHX} POST_BUILD
    COMMAND ${SDCC_MAKEBIN} -p $<TARGET_FILE:${TARGET_IHX}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin (makebin)")
