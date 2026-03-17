set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR 8051)

# ===========================================================================
# BinaryKeyboard — CH552G MCS-51 Cross-Compilation Toolchain (SDCC)
# ===========================================================================
#
# This file auto-detects the SDCC (Small Device C Compiler) toolchain.
#
# Detection order:
#   1. SDCC_ROOT       — cmake -D or env var, pointing to SDCC install root
#   2. System PATH     — auto-discover sdcc from PATH
#
# Install SDCC:
#   macOS:   brew install sdcc
#   Linux:   apt install sdcc    (or pacman -S sdcc)
#   Windows: https://sdcc.sourceforge.net  (add bin/ to PATH)
#
# Quick start:
#   cmake --preset local-release && cmake --build --preset local-release
#
# ===========================================================================

# --- Absorb env vars --------------------------------------------------------
if(NOT DEFINED SDCC_ROOT AND DEFINED ENV{SDCC_ROOT})
    file(TO_CMAKE_PATH "$ENV{SDCC_ROOT}" SDCC_ROOT)
    set(SDCC_ROOT "${SDCC_ROOT}" CACHE PATH "SDCC install root directory")
endif()

# --- Find the compiler ------------------------------------------------------
unset(_sdcc_found CACHE)

# Pass 1: hint from SDCC_ROOT
if(DEFINED SDCC_ROOT)
    find_program(_sdcc_found "sdcc" HINTS "${SDCC_ROOT}/bin" NO_DEFAULT_PATH)
endif()

# Pass 2: system PATH
if(NOT _sdcc_found)
    find_program(_sdcc_found "sdcc")
endif()

if(NOT _sdcc_found)
    message(FATAL_ERROR
        "\n"
        "  ── BinaryKeyboard / 二进制键盘 ───────────────────────────────\n"
        "\n"
        "  SDCC (Small Device C Compiler) not found.\n"
        "  未找到 SDCC 编译器。\n"
        "\n"
        "  Install:\n"
        "    macOS:   brew install sdcc\n"
        "    Linux:   apt install sdcc\n"
        "    Windows: https://sdcc.sourceforge.net\n"
        "\n"
        "  Or set SDCC_ROOT to the SDCC install directory.\n"
        "\n"
        "  ───────────────────────────────────────────────────────────────\n")
endif()

get_filename_component(SDCC_BIN_DIR "${_sdcc_found}" DIRECTORY)
unset(_sdcc_found CACHE)

message(STATUS "SDCC toolchain: ${SDCC_BIN_DIR}/sdcc")

if(CMAKE_HOST_WIN32)
    set(_sdcc_exe_suffix ".exe")
else()
    set(_sdcc_exe_suffix "")
endif()

# ---------------------------------------------------------------------------
# Tool paths
# ---------------------------------------------------------------------------
set(CMAKE_C_COMPILER   "${SDCC_BIN_DIR}/sdcc${_sdcc_exe_suffix}"      CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${SDCC_BIN_DIR}/sdas8051${_sdcc_exe_suffix}"  CACHE FILEPATH "" FORCE)
set(SDCC_PACKIHX       "${SDCC_BIN_DIR}/packihx${_sdcc_exe_suffix}"   CACHE FILEPATH "" FORCE)
set(SDCC_MAKEBIN       "${SDCC_BIN_DIR}/makebin${_sdcc_exe_suffix}"   CACHE FILEPATH "" FORCE)
set(SDCC_SDOBJCOPY     "${SDCC_BIN_DIR}/sdobjcopy${_sdcc_exe_suffix}" CACHE FILEPATH "" FORCE)

# Forward toolchain variables into try_compile subprojects
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    SDCC_ROOT
    SDCC_BIN_DIR
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_C_COMPILER_WORKS TRUE CACHE INTERNAL "")
set(CMAKE_C_COMPILER_FORCED TRUE CACHE INTERNAL "")
set(CMAKE_ASM_COMPILER_WORKS TRUE CACHE INTERNAL "")
set(CMAKE_ASM_COMPILER_FORCED TRUE CACHE INTERNAL "")
set(CMAKE_C_OUTPUT_EXTENSION ".rel")
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_ASM_OUTPUT_EXTENSION ".rel")
set(CMAKE_ASM_OUTPUT_EXTENSION_REPLACE 1)

# ---------------------------------------------------------------------------
# CH552G memory layout
# ---------------------------------------------------------------------------
# Flash:  16 KB total, ~2 KB bootloader → 14336 bytes user code
# IRAM:   256 bytes (8051 internal)
# XRAM:   1024 bytes, first USER_USB_RAM bytes reserved for USB buffers
set(CH552_CODE_SIZE  "14208" CACHE STRING "Code (Flash) size in bytes – last 128 B reserved for macros (2 × 64 B)" FORCE)
set(CH552_XRAM_SIZE  "1024"  CACHE STRING "Total XRAM size in bytes")
set(CH552_USB_RAM    "148"   CACHE STRING "XRAM bytes reserved for USB buffers")
math(EXPR CH552_USER_XRAM "${CH552_XRAM_SIZE} - ${CH552_USB_RAM}")

# ---------------------------------------------------------------------------
# CPU / architecture flags
# ---------------------------------------------------------------------------
set(CMAKE_C_FLAGS_INIT "-mmcs51 --model-small --std-c11 --opt-code-size")

set(_BK_CH552_LINK_FLAGS
    "-mmcs51 --model-small --code-size ${CH552_CODE_SIZE} --xram-loc ${CH552_USB_RAM} --xram-size ${CH552_USER_XRAM}")

# Keep existing build directories consistent when CH552_CODE_SIZE is changed.
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_BK_CH552_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${_BK_CH552_LINK_FLAGS}" CACHE STRING "SDCC linker flags" FORCE)

set(CMAKE_C_FLAGS_DEBUG          "--debug" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELEASE        ""        CACHE INTERNAL "")
set(CMAKE_C_FLAGS_MINSIZEREL     "--opt-code-size" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "--debug" CACHE INTERNAL "")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
