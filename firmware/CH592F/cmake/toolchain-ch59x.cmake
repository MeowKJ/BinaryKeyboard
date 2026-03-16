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
#   1. MRS_TOOLCHAIN_ROOT  — cmake -D / env var / console cache
#   2. TOOLCHAIN_DIR       — direct path to compiler bin/
#   3. Cached riscv_gcc    — tools/scripts/.binarykeyboard_console_state.json
#   4. System PATH         — auto-discover from PATH
#
# Quick start:
#   python tools/scripts/console.py
#   # Configure toolchain once, then:
#   cmake --preset release-5key && cmake --build --preset release-5key
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

# --- Shared console cache ---------------------------------------------------
file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../.." _bk_repo_root)
set(_bk_state_file "${_bk_repo_root}/tools/scripts/.binarykeyboard_console_state.json")
if(EXISTS "${_bk_state_file}")
    file(READ "${_bk_state_file}" _bk_state_json)
endif()

if(NOT DEFINED MRS_TOOLCHAIN_ROOT AND DEFINED _bk_state_json)
    string(JSON _bk_toolchain_root ERROR_VARIABLE _bk_toolchain_root_error GET "${_bk_state_json}" toolchain_root)
    if(NOT _bk_toolchain_root_error AND _bk_toolchain_root AND NOT _bk_toolchain_root STREQUAL "null")
        file(TO_CMAKE_PATH "${_bk_toolchain_root}" MRS_TOOLCHAIN_ROOT)
        set(MRS_TOOLCHAIN_ROOT "${MRS_TOOLCHAIN_ROOT}" CACHE PATH "MounRiver Toolchain root directory")
    endif()
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

# --- Cached riscv_gcc path → TOOLCHAIN_DIR ----------------------------------
if(NOT DEFINED TOOLCHAIN_DIR AND DEFINED _bk_state_json)
    string(JSON _bk_cached_gcc ERROR_VARIABLE _bk_cached_gcc_error GET "${_bk_state_json}" tool_cache riscv_gcc)
    if(NOT _bk_cached_gcc_error AND _bk_cached_gcc AND NOT _bk_cached_gcc STREQUAL "null")
        file(TO_CMAKE_PATH "${_bk_cached_gcc}" _bk_cached_gcc_path)
        if(EXISTS "${_bk_cached_gcc_path}")
            get_filename_component(_bk_cached_gcc_dir "${_bk_cached_gcc_path}" DIRECTORY)
            set(TOOLCHAIN_DIR "${_bk_cached_gcc_dir}" CACHE PATH "RISC-V toolchain bin directory" FORCE)
        endif()
    endif()
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
        "    A) Run python tools/scripts/console.py once and use 'Configure toolchain'.\n"
        "\n"
        "    B) Set MRS_TOOLCHAIN_ROOT in your shell or pass -DMRS_TOOLCHAIN_ROOT=...\n"
        "\n"
        "    C) Add the compiler bin/ directory to your system PATH.\n"
        "\n"
        "  Download: https://www.mounriver.com/download\n"
        "\n"
        "  ───────────────────────────────────────────────────────────────\n")
endif()

message(STATUS "Toolchain: ${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc")

if(CMAKE_HOST_WIN32)
    set(_tool_exe_suffix ".exe")
else()
    set(_tool_exe_suffix "")
endif()

# ---------------------------------------------------------------------------
# Tool paths
# ---------------------------------------------------------------------------
set(CMAKE_C_COMPILER   "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc${_tool_exe_suffix}"     CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}gcc${_tool_exe_suffix}"     CACHE FILEPATH "" FORCE)
set(CROSS_OBJDUMP      "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objdump${_tool_exe_suffix}" CACHE FILEPATH "" FORCE)
set(CROSS_OBJCOPY      "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}objcopy${_tool_exe_suffix}" CACHE FILEPATH "" FORCE)
set(CROSS_SIZE         "${TOOLCHAIN_DIR}/${TOOLCHAIN_PREFIX}size${_tool_exe_suffix}"    CACHE FILEPATH "" FORCE)

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
set(CMAKE_ASM_FLAGS_INIT        "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

set(CMAKE_C_FLAGS_DEBUG          "-Os -g3" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELEASE        "-O3"     CACHE INTERNAL "")
set(CMAKE_C_FLAGS_MINSIZEREL     "-Os"     CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g"  CACHE INTERNAL "")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
