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
        VERBATIM
    )
endfunction()

set(CH552_BUILD_TARGETS)
foreach(keyboard IN ITEMS BASIC KNOB 5KEY)
    string(TOLOWER "${keyboard}" keyboard_slug)
    set(target_name "ch552_${keyboard_slug}")
    add_script_build_target(
        ${target_name}
        SCRIPT ${SCRIPT_ROOT}/ch552g.py
        SCRIPT_ARGS build --keyboard ${keyboard}
    )
    list(APPEND CH552_BUILD_TARGETS ${target_name})
endforeach()

if(NOT CH592_PROFILE)
    set(CH592_PROFILE "release")
endif()

set(CH592_BUILD_TARGETS)
foreach(keyboard IN ITEMS 5KEY KNOB)
    string(TOLOWER "${keyboard}" keyboard_slug)
    add_script_build_target(
        ch592_${keyboard_slug}
        SCRIPT ${SCRIPT_ROOT}/ch592f.py
        SCRIPT_ARGS build --keyboard ${keyboard} --profile ${CH592_PROFILE}
    )
    list(APPEND CH592_BUILD_TARGETS ch592_${keyboard_slug})
endforeach()

add_custom_target(ch552_all DEPENDS ${CH552_BUILD_TARGETS})
add_custom_target(ch592_all DEPENDS ${CH592_BUILD_TARGETS})
add_custom_target(firmware_all DEPENDS ch552_all ch592_all)
