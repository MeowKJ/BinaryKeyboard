cmake_minimum_required(VERSION 3.21)

# Must be set before project()
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-ch59x.cmake)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CH592F C ASM)

set(TARGET_ELF ${PROJECT_NAME}.elf)

# ---------------------------------------------------------------------------
# Device naming (used by both USB and BLE)
# ---------------------------------------------------------------------------
set(KBD_NAME_PREFIX "BinaryKeyboard" CACHE STRING "Keyboard name prefix")
set(KBD_MODEL "5KEY" CACHE STRING "Keyboard model suffix for device name (e.g. 5KEY, 65KEY)")
set(KBD_DEVICE_NAME_OVERRIDE "" CACHE STRING "Override full USB/BLE device name, empty means auto compose")
string(TOUPPER "${KBD_MODEL}" KBD_MODEL_UPPER)

if(KBD_DEVICE_NAME_OVERRIDE STREQUAL "")
    set(KBD_DEVICE_NAME "${KBD_NAME_PREFIX}${KBD_MODEL_UPPER}")
else()
    set(KBD_DEVICE_NAME "${KBD_DEVICE_NAME_OVERRIDE}")
endif()

string(LENGTH "${KBD_DEVICE_NAME}" KBD_DEVICE_NAME_LEN)

message(STATUS "Keyboard model: ${KBD_MODEL_UPPER}")
message(STATUS "Unified device name: ${KBD_DEVICE_NAME}")
message(STATUS "Unified device name length: ${KBD_DEVICE_NAME_LEN}")

if(KBD_DEVICE_NAME_LEN GREATER 31)
    message(WARNING "Device name length > 31, USB product string will be truncated at runtime.")
endif()

if(KBD_DEVICE_NAME_LEN GREATER 29)
    message(WARNING "Device name length > 29, BLE scan response local name will be shortened.")
elseif(KBD_DEVICE_NAME_LEN GREATER 21)
    message(WARNING "Device name length > 21, BLE GATT Device Name will be truncated.")
endif()

# ---------------------------------------------------------------------------
# Source files
# Excluded (per CH592F.wvproj):
#   StdPeriphDriver/CH59x_usbhostClass.c  CH59x_usbhostBase.c
#   StdPeriphDriver/CH59x_uart2.c  CH59x_uart3.c  CH59x_spi0.c  CH59x_pwm.c
#   StdPeriphDriver/CH59x_i2c.c (project unused, avoid vendor warning noise)
#   StdPeriphDriver/CH59x_flash.c CH59x_lcd.c CH59x_timer2.c CH59x_timer3.c
#   StdPeriphDriver/CH59x_uart0.c CH59x_uart1.c (unused in current build; APIs resolved elsewhere / not enabled)
#   MeowBLE/core/src/KEY.c  LED.c
#   MeowBLE/core/src/ble_sleep.c (HAL_SLEEP = FALSE in current config)
#   User/Main_Test.c  User/RGB_TEST.c
#   RVMSIS/core_riscv.c (current firmware only uses header inlines/macros)
# ---------------------------------------------------------------------------
set(SOURCES
    # Entry point
    User/Main.c

    # Board / HAL
    Board/src/debug.c
    Board/src/hal_utils.c
    Board/src/kbd_battery.c
    Board/src/key.c
    Board/src/ws2812.c

    # MeowBLE – core
    MeowBLE/core/src/ble_mcu.c
    MeowBLE/core/src/ble_rtc.c

    # MeowBLE – HID
    MeowBLE/hid/src/ble_hid.c
    MeowBLE/hid/src/ble_hid_service.c
    MeowBLE/hid/src/kbd_mode.c

    # MeowBLE – GATT profiles
    MeowBLE/profile/src/battservice.c
    MeowBLE/profile/src/devinfoservice.c
    MeowBLE/profile/src/hiddev.c
    MeowBLE/profile/src/scanparamservice.c

    # MeowKeyboard
    MeowKeyboard/src/kbd_command.c
    MeowKeyboard/src/kbd_core.c
    MeowKeyboard/src/kbd_log.c
    MeowKeyboard/src/kbd_rgb.c
    MeowKeyboard/src/kbd_storage.c

    # MeowUSB
    MeowUSB/src/usb_descriptors.c
    MeowUSB/src/usb_device.c
    MeowUSB/src/usb_hid.c

    # WCH Standard Peripheral Driver
    StdPeriphDriver/CH59x_adc.c
    StdPeriphDriver/CH59x_clk.c
    StdPeriphDriver/CH59x_gpio.c
    StdPeriphDriver/CH59x_pwr.c
    StdPeriphDriver/CH59x_sys.c
    StdPeriphDriver/CH59x_timer0.c
    StdPeriphDriver/CH59x_timer1.c
    StdPeriphDriver/CH59x_usbdev.c

    # Startup (assembly)
    Startup/startup_CH592.S
    Startup/ble_task_scheduler.S
)

add_executable(${TARGET_ELF} ${SOURCES})

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(${TARGET_ELF} PRIVATE
    StdPeriphDriver/inc
    RVMSIS
    Board/include
    MeowUSB/include
    MeowBLE/lib
    MeowBLE/hid/include
    MeowBLE/profile/include
    MeowBLE/core/include
    MeowKeyboard/include
)

# ---------------------------------------------------------------------------
# Compiler options
# ---------------------------------------------------------------------------
target_compile_definitions(${TARGET_ELF} PRIVATE
    DEBUG=1
    KBD_MODEL_NAME=\"${KBD_MODEL_UPPER}\"
    KBD_DEVICE_NAME=\"${KBD_DEVICE_NAME}\"
)

target_compile_options(${TARGET_ELF} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=gnu99 -Wall -Wunused -Wuninitialized>
)

# ---------------------------------------------------------------------------
# Linker options
# ---------------------------------------------------------------------------
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/Ld/Link.ld)

target_link_options(${TARGET_ELF} PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
)

# Pre-compiled vendor libraries
target_link_libraries(${TARGET_ELF} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/StdPeriphDriver/libISP592.a
    ${CMAKE_CURRENT_SOURCE_DIR}/MeowBLE/lib/libCH59xBLE.a
)

# ---------------------------------------------------------------------------
# Post-build: generate .bin and .hex, print size
# ---------------------------------------------------------------------------
add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET_ELF}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.bin")

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_OBJCOPY} -Oihex $<TARGET_FILE:${TARGET_ELF}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMENT "Generating ${PROJECT_NAME}.hex")

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND ${CROSS_SIZE} --format=berkeley $<TARGET_FILE:${TARGET_ELF}>
    COMMENT "Firmware size:")
