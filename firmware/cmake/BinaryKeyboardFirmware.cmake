include_guard(GLOBAL)

find_package(Python3 COMPONENTS Interpreter REQUIRED)


function(bk_add_version_header)
    set(options)
    set(oneValueArgs CHIP TARGET_NAME GENERATED_DIR_VAR HEADER_VAR)
    cmake_parse_arguments(BK "${options}" "${oneValueArgs}" "" ${ARGN})

    foreach(required IN ITEMS CHIP TARGET_NAME GENERATED_DIR_VAR HEADER_VAR)
        if(NOT BK_${required})
            message(FATAL_ERROR "bk_add_version_header missing required argument: ${required}")
        endif()
    endforeach()

    set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(_header "${_generated_dir}/bk_version_config.h")

    add_custom_command(
        OUTPUT "${_header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_generated_dir}"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/scripts/versioning.py"
                emit-c-header --chip "${BK_CHIP}" --out "${_header}"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/scripts/versioning.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../config/versions.json"
        COMMENT "Generating ${BK_CHIP} version header"
        VERBATIM
    )

    add_custom_target(${BK_TARGET_NAME} DEPENDS "${_header}")

    set(${BK_GENERATED_DIR_VAR} "${_generated_dir}" PARENT_SCOPE)
    set(${BK_HEADER_VAR} "${_header}" PARENT_SCOPE)
endfunction()
