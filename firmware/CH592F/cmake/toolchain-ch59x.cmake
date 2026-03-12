set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# ===========================================================================
# BinaryKeyboard — CH592F RISC-V Cross-Compilation Toolchain
# ===========================================================================
#
# This file auto-detects the MounRiver Studio (MRS) RISC-V GCC toolchain.
# Supported compiler prefixes:
#   riscv-none-embed-  |  riscv-wch-elf-  |  riscv-none-elf-
#
# Detection order:
#   1. MRS_TOOLCHAIN_ROOT  — CMakeUserPresets.json / cmake -D / env var
#   2. TOOLCHAIN_DIR       — direct path to compiler bin/
#   3. System PATH         — auto-discover from PATH
#
# Quick start:
#   cp CMakeUserPresets.json.example CMakeUserPresets.json
#   # Edit MRS_TOOLCHAIN_ROOT, then:
#   cmake --preset local-release && cmake --build --preset local-release
#
# Download: https://www.mounriver.com/download
# ===========================================================================

# --- Absorb env vars --------------------------------------------------------
if(NOT DEFINED TOOLCHAIN_DIR AND DEFINED ENV{RISCV_TOOLCHAIN_DIR})
    file(TO_CMAKE_PATH "$ENV{RISCV_TOOLCHAIN_DIR}" TOOLCHAIN_DIR)
    set(TOOLCHAIN_DIR "${TOOLCHAIN_DIR}" CACHE PATH "RISC-V toolchain bin directory")
endif()

if(NOT DEFINED MRS_TOOLCHAIN_ROOT AND DEFINED ENV{MRS_TOOLCHAIN_ROOT})
    file(TO_CMAKE_PATH "$ENV{MRS_TOOLCHAIN_ROOT}" MRS_TOOLCHAIN_ROOT)
    set(MRS_TOOLCHAIN_ROOT "${MRS_TOOLCHAIN_ROOT}" CACHE PATH "MounRiver Toolchain root directory")
endif()

# --- MRS_TOOLCHAIN_ROOT → TOOLCHAIN_DIR ------------------------------------
if(NOT DEFINED TOOLCHAIN_DIR AND DEFINED MRS_TOOLCHAIN_ROOT)
    foreach(_sub IN ITEMS "RISC-V Embedded GCC/bin" "RISC-V Embedded GCC12/bin")
        if(EXISTS "${MRS_TOOLCHAIN_ROOT}/${_sub}")
            set(TOOLCHAIN_DIR "${MRS_TOOLCHAIN_ROOT}/${_sub}"
                CACHE PATH "RISC-V toolchain bin directory" FORCE)
            break()
        endif()
    endforeach()
endif()

# --- Find the compiler ------------------------------------------------------
# Search order:
#   1) TOOLCHAIN_DIR hint (if provided)
#   2) system PATH fallback (prevents stale cache paths from hard-failing)
unset(TOOLCHAIN_PREFIX CACHE)

foreach(_pass IN ITEMS "hint" "path")
    if(DEFINED TOOLCHAIN_PREFIX)
        break()
    endif()

    if(_pass STREQUAL "hint")
        if(DEFINED TOOLCHAIN_DIR)
            set(_find_args HINTS "${TOOLCHAIN_DIR}" NO_DEFAULT_PATH)
        else()
            continue()
        endif()
    else()
        set(_find_args "")
    endif()

    foreach(_prefix IN ITEMS "riscv-none-embed-" "riscv-wch-elf-" "riscv-none-elf-")
        find_program(_probe_gcc "${_prefix}gcc" ${_find_args})
        if(_probe_gcc)
            get_filename_component(_bin_dir "${_probe_gcc}" DIRECTORY)
            set(TOOLCHAIN_DIR    "${_bin_dir}" CACHE PATH   "RISC-V toolchain bin directory" FORCE)
            set(TOOLCHAIN_PREFIX "${_prefix}"  CACHE STRING "RISC-V toolchain prefix"        FORCE)
            unset(_probe_gcc CACHE)
            break()
        endif()
        unset(_probe_gcc CACHE)
    endforeach()
endforeach()

if(NOT DEFINED TOOLCHAIN_PREFIX)
    message(FATAL_ERROR
        "\n"
        "  ── BinaryKeyboard / 二进制键盘 ───────────────────────────────\n"
        "\n"
        "  CH592F RISC-V cross-compiler not found.\n"
        "  未找到 CH592F RISC-V 交叉编译器。\n"
        "\n"
        "  Searched: riscv-none-embed-gcc / riscv-wch-elf-gcc / riscv-none-elf-gcc\n"
        "\n"
        "  Fix (choose one):\n"
        "\n"
        "    A) Set MRS_TOOLCHAIN_ROOT in CMakeUserPresets.json:\n"
        "       cp CMakeUserPresets.json.example CMakeUserPresets.json\n"
        "       Then edit MRS_TOOLCHAIN_ROOT to your toolchain path.\n"
        "\n"
        "    B) Add the compiler bin/ directory to your system PATH.\n"
        "\n"
        "  Download: https://www.mounriver.com/download\n"
        "\n"
        "  ───────────────────────────────────────────────────────────────\n")
endif()

message(STATUS "Toolchain: ${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc")

# ---------------------------------------------------------------------------
# Tool paths
# ---------------------------------------------------------------------------
set(CMAKE_C_COMPILER   "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc"     CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}g++"     CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc"     CACHE FILEPATH "" FORCE)
set(CROSS_OBJDUMP      "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objdump" CACHE FILEPATH "" FORCE)
set(CROSS_OBJCOPY      "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objcopy" CACHE FILEPATH "" FORCE)
set(CROSS_SIZE         "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}size"    CACHE FILEPATH "" FORCE)

# Forward toolchain variables into try_compile subprojects so the
# find_program search above succeeds even during ABI-detection builds.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    TOOLCHAIN_DIR
    TOOLCHAIN_PREFIX
    MRS_TOOLCHAIN_ROOT
)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---------------------------------------------------------------------------
# CPU / architecture flags
# ---------------------------------------------------------------------------
if(TOOLCHAIN_PREFIX STREQUAL "riscv-none-embed-")
    set(_cpu_march "rv32imac")
else()
    set(_cpu_march "rv32imac_zicsr_zifencei")
endif()

set(CPU_FLAGS "-march=${_cpu_march} -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore")

set(_common "${CPU_FLAGS} -ffunction-sections -fdata-sections -fno-common")
set(CMAKE_C_FLAGS_INIT          "${_common}")
set(CMAKE_CXX_FLAGS_INIT        "${_common} -fno-rtti -fno-exceptions")
set(CMAKE_ASM_FLAGS_INIT        "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

set(CMAKE_C_FLAGS_DEBUG          "-Og -g3" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELEASE        "-O3"     CACHE INTERNAL "")
set(CMAKE_C_FLAGS_MINSIZEREL     "-Os"     CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g"  CACHE INTERNAL "")

set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g3" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3"     CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os"     CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g"  CACHE INTERNAL "")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
