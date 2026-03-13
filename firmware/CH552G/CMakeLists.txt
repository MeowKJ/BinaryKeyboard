cmake_minimum_required(VERSION 3.21)

# Must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-ch552.cmake)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CH552G C)

set(TARGET_IHX ${PROJECT_NAME}.ihx)

# ---------------------------------------------------------------------------
# Device variant selection
# ---------------------------------------------------------------------------
set(KBD_VARIANT "BASIC" CACHE STRING "Keyboard variant: BASIC, KNOB, 5KEYS")
set_property(CACHE KBD_VARIANT PROPERTY STRINGS "BASIC" "KNOB" "5KEYS")
string(TOUPPER "${KBD_VARIANT}" KBD_VARIANT_UPPER)
message(STATUS "Keyboard variant: ${KBD_VARIANT_UPPER}")

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

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(${TARGET_IHX} PRIVATE
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
    USE_${KBD_VARIANT_UPPER}
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
