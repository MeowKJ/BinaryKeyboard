cmake_minimum_required(VERSION 3.21)

# Must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-ch552.cmake)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CH552G C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/BinaryKeyboardFirmware.cmake)

set(TARGET_IHX ${PROJECT_NAME}.ihx)

bk_add_version_header(
    CHIP CH552G
    TARGET_NAME ch552g_version_header
    GENERATED_DIR_VAR GENERATED_DIR
    HEADER_VAR BK_VERSION_HEADER
)

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
set(CH552_RUNTIME_SOURCES
    SDK/cores/main.c
    app/main.c
)

set(CH552_HAL_SOURCES
    hal/hal.c
)

set(CH552_KEYBOARD_SOURCES
    keyboard/KeyScanner.c
    keyboard/KeysDataHandler.c
    keyboard/EncoderHandler.c
    keyboard/MacroStorage.c
    keyboard/rgb.c
    keyboard/LightingController.c
)

set(CH552_USB_SOURCES
    usb/CustomUSBHID.c
    usb/USBConstant.c
    usb/USBHandler.c
)

set(CH552_SDK_CORE_SOURCES
    SDK/cores/wiring.c
    SDK/cores/eeprom.c
)

set(CH552_LED_SOURCES
    SDK/libraries/WS2812/template/optionalLink_WS2812_P1_5.c
)

set(CH552_SOURCES
    ${CH552_RUNTIME_SOURCES}
    ${CH552_HAL_SOURCES}
    ${CH552_KEYBOARD_SOURCES}
    ${CH552_USB_SOURCES}
    ${CH552_SDK_CORE_SOURCES}
    ${CH552_LED_SOURCES}
)

add_executable(${TARGET_IHX} ${CH552_SOURCES})
add_dependencies(${TARGET_IHX} ch552g_version_header)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CH552_SOURCES})

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
set(CH552_INCLUDE_DIRS
    ${GENERATED_DIR}
    # ch55xduino SDK headers
    SDK/cores
    SDK/variants/ch552
    SDK/libraries/WS2812
    SDK/libraries/WS2812/template
    # Application headers
    app
    hal
    keyboard
    usb
)

target_include_directories(${TARGET_IHX} PRIVATE ${CH552_INCLUDE_DIRS})

# ---------------------------------------------------------------------------
# Compiler definitions
# ---------------------------------------------------------------------------
set(CH552_COMPILE_DEFINITIONS
    CH552=1
    F_CPU=24000000
    USER_USB_RAM=${CH552_USB_RAM}
    ${KEYBOARD_DEFINE}
)

target_compile_definitions(${TARGET_IHX} PRIVATE ${CH552_COMPILE_DEFINITIONS})

# ---------------------------------------------------------------------------
# Compiler options
# ---------------------------------------------------------------------------
set(CH552_COMPILE_OPTIONS
    --less-pedantic
)

target_compile_options(${TARGET_IHX} PRIVATE ${CH552_COMPILE_OPTIONS})

# ---------------------------------------------------------------------------
# Artifact export: generate .hex and .bin as explicit build outputs
# ---------------------------------------------------------------------------
set(TARGET_HEX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(TARGET_BIN ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(
    OUTPUT ${TARGET_HEX}
    COMMAND ${SDCC_SDOBJCOPY} -I ihex -O ihex $<TARGET_FILE:${TARGET_IHX}> ${TARGET_HEX}
    DEPENDS ${TARGET_IHX}
    COMMENT "Generating ${PROJECT_NAME}.hex (sdobjcopy)"
    VERBATIM
)

add_custom_command(
    OUTPUT ${TARGET_BIN}
    COMMAND ${SDCC_SDOBJCOPY} -I ihex -O binary $<TARGET_FILE:${TARGET_IHX}> ${TARGET_BIN}
    DEPENDS ${TARGET_IHX}
    COMMENT "Generating ${PROJECT_NAME}.bin (sdobjcopy)"
    VERBATIM
)

add_custom_target(${PROJECT_NAME}_artifacts ALL DEPENDS ${TARGET_HEX} ${TARGET_BIN})
