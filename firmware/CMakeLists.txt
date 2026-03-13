cmake_minimum_required(VERSION 3.21)

project(BinaryKeyboardFirmwareSuperbuild NONE)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SCRIPT_ROOT ${PROJECT_ROOT}/tools/scripts)

function(add_script_build_target TARGET_NAME)
    set(options)
    set(oneValueArgs SCRIPT)
    set(multiValueArgs SCRIPT_ARGS)
    cmake_parse_arguments(SUB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_custom_target(${TARGET_NAME}
        COMMAND ${Python3_EXECUTABLE} ${SUB_SCRIPT} ${SUB_SCRIPT_ARGS}
        WORKING_DIRECTORY ${PROJECT_ROOT}
        USES_TERMINAL
        COMMENT "Building ${TARGET_NAME}"
    )
endfunction()

add_script_build_target(
    ch552_basic
    SCRIPT ${SCRIPT_ROOT}/ch552g.py
    SCRIPT_ARGS build --keyboard BASIC
)

add_script_build_target(
    ch552_knob
    SCRIPT ${SCRIPT_ROOT}/ch552g.py
    SCRIPT_ARGS build --keyboard KNOB
)

add_script_build_target(
    ch552_5keys
    SCRIPT ${SCRIPT_ROOT}/ch552g.py
    SCRIPT_ARGS build --keyboard 5KEY
)

add_script_build_target(
    ch592_5key
    SCRIPT ${SCRIPT_ROOT}/ch592f.py
    SCRIPT_ARGS build --keyboard 5KEY --profile release
)

add_script_build_target(
    ch592_knob
    SCRIPT ${SCRIPT_ROOT}/ch592f.py
    SCRIPT_ARGS build --keyboard KNOB --profile release
)
