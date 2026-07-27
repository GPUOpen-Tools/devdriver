## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

include_guard()

# FORCE to ensure this value is always up to date.
# NOTE: In CMake 3.17 use CMAKE_CURRENT_FUNCTION_LIST_DIR instead.
set(DD_MODULE_UTIL_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "" FORCE)

function(_dd_validate_module_generator_parameter name value)
    if(NOT DEFINED ${value})
        message(FATAL_ERROR "${name} must be provided in order to generate a module!")
    endif()
endfunction()

# Reduces the amount of boilerplate code clients need to write for devdriver modules.
function(dd_generate_boilerplate_module_code)
    set(oneValueArgs
        BASE_MODULE_NAME
        TARGET_NAME
        MODULE_NAME
        MODULE_DESCRIPTION
        MODULE_VERSION_MAJOR
        MODULE_VERSION_MINOR
        MODULE_VERSION_PATCH
        MODULE_NAMESPACE
        SYSTEM_CONTEXT_CLASS_NAME
        CLIENT_CONTEXT_CLASS_NAME
        DATA_CONTEXT_CLASS_NAME
        CONNECTION_CONTEXT_CLASS_NAME
        COMMAND_CONTEXT_CLASS_NAME
    )
    cmake_parse_arguments(DD_GEN_MODULE_ARGS "" "${oneValueArgs}" "" ${ARGN})

    if(DD_GEN_MODULE_ARGS_BASE_MODULE_NAME)
        set(DD_GEN_MODULE_IS_EXTENSION_MODULE ON)
    endif()

    if(NOT DD_GEN_MODULE_IS_EXTENSION_MODULE)
        _dd_validate_module_generator_parameter(TARGET_NAME DD_GEN_MODULE_ARGS_TARGET_NAME)
        _dd_validate_module_generator_parameter(MODULE_NAME DD_GEN_MODULE_ARGS_MODULE_NAME)
        _dd_validate_module_generator_parameter(MODULE_DESCRIPTION DD_GEN_MODULE_ARGS_MODULE_DESCRIPTION)
        _dd_validate_module_generator_parameter(MODULE_VERSION_MAJOR DD_GEN_MODULE_ARGS_MODULE_VERSION_MAJOR)
        _dd_validate_module_generator_parameter(MODULE_VERSION_MINOR DD_GEN_MODULE_ARGS_MODULE_VERSION_MINOR)
        _dd_validate_module_generator_parameter(MODULE_VERSION_PATCH DD_GEN_MODULE_ARGS_MODULE_VERSION_PATCH)
        _dd_validate_module_generator_parameter(MODULE_NAMESPACE DD_GEN_MODULE_ARGS_MODULE_NAMESPACE)

        string(TOUPPER ${DD_GEN_MODULE_ARGS_MODULE_NAME} DD_GEN_MODULE_MODULE_NAME_UPPER)

        if (DD_GEN_MODULE_ARGS_SYSTEM_CONTEXT_CLASS_NAME)
            set(DD_GEN_MODULE_SUPPORTS_SYSTEM_CONTEXT ON)
        endif()

        if (DD_GEN_MODULE_ARGS_CLIENT_CONTEXT_CLASS_NAME)
            set(DD_GEN_MODULE_SUPPORTS_CLIENT_CONTEXT ON)
        endif()

        if (DD_GEN_MODULE_ARGS_DATA_CONTEXT_CLASS_NAME)
            set(DD_GEN_MODULE_SUPPORTS_DATA_CONTEXT ON)
        endif()

        if (DD_GEN_MODULE_ARGS_CONNECTION_CONTEXT_CLASS_NAME)
            set(DD_GEN_MODULE_SUPPORTS_CONNECTION_CONTEXT ON)
        endif()

        if (DD_GEN_MODULE_ARGS_COMMAND_CONTEXT_CLASS_NAME)
            set(DD_GEN_MODULE_SUPPORTS_COMMAND_CONTEXT ON)
        endif()
    else()
        _dd_validate_module_generator_parameter(BASE_MODULE_NAME DD_GEN_MODULE_ARGS_BASE_MODULE_NAME)
        _dd_validate_module_generator_parameter(TARGET_NAME DD_GEN_MODULE_ARGS_TARGET_NAME)
        _dd_validate_module_generator_parameter(MODULE_NAME DD_GEN_MODULE_ARGS_MODULE_NAME)
        _dd_validate_module_generator_parameter(MODULE_NAMESPACE DD_GEN_MODULE_ARGS_MODULE_NAMESPACE)
    endif()

    # Generate the module interface code
    configure_file(${DD_MODULE_UTIL_DIR}/ModuleInterface.h.in generated/inc/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleInterface.h)
    configure_file(${DD_MODULE_UTIL_DIR}/ModuleInterface.cpp.in generated/src/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleInterface.cpp)

    target_include_directories(${DD_GEN_MODULE_ARGS_TARGET_NAME}
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}/generated/inc
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/generated/src
    )

    target_sources(${DD_GEN_MODULE_ARGS_TARGET_NAME}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/generated/inc/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleInterface.h
            ${CMAKE_CURRENT_BINARY_DIR}/generated/src/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleInterface.cpp
    )

    # WA: We rename the user library's output file to avoid conflicts with the generated dynamic module target.
    #     If the user's library is named "{X}Module", the generated .lib file will conflict with the dynamic module import lib
    #     and cause confusing linker errors.
    set_target_properties(${DD_GEN_MODULE_ARGS_TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME ${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleUserLib
    )

    # Generate the static export target
    configure_file(${DD_MODULE_UTIL_DIR}/ModuleStatic.h.in generated/inc/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleStatic.h)
    configure_file(${DD_MODULE_UTIL_DIR}/ModuleStatic.cpp.in generated/src/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleStatic.cpp)

    set(g_ModuleStatic "${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleStatic")

    devdriver_library(${g_ModuleStatic} STATIC)

    target_link_libraries(${g_ModuleStatic}
        PUBLIC
            ddApi
            ddModule
        PRIVATE
            ${DD_GEN_MODULE_ARGS_TARGET_NAME}
    )

    target_include_directories(${g_ModuleStatic}
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}/generated/inc
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/generated/src
    )

    target_sources(${g_ModuleStatic} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated/inc/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleStatic.h
        ${CMAKE_CURRENT_BINARY_DIR}/generated/src/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleStatic.cpp
    )

    # Generate the dynamic export target
    configure_file(${DD_MODULE_UTIL_DIR}/ModuleDynamic.cpp.in generated/src/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleDynamic.cpp)

    set(g_ModuleShared "${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleDynamic")

    devdriver_library(${g_ModuleShared} MODULE)
    target_link_libraries(${g_ModuleShared}
        PRIVATE
            ${g_ModuleStatic}
            ddModule
    )
    set_target_properties(${g_ModuleShared}
        PROPERTIES
            OUTPUT_NAME ${DD_GEN_MODULE_ARGS_MODULE_NAME}Module
    )

    # Ensure that the module is configured to export symbols
    target_compile_definitions(${g_ModuleShared} PRIVATE DDLIB_EXPORTS)

    target_include_directories(${g_ModuleShared}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/generated/src
    )

    target_sources(${g_ModuleShared} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated/src/g_${DD_GEN_MODULE_ARGS_MODULE_NAME}ModuleDynamic.cpp
    )

    set_target_properties(${DD_GEN_MODULE_ARGS_TARGET_NAME} ${g_ModuleShared} ${g_ModuleStatic}
        PROPERTIES
            FOLDER "${CMAKE_FOLDER}/${DD_GEN_MODULE_ARGS_MODULE_NAME}Module"
    )

    if (DD_BP_INSTALL)
        install(TARGETS ${g_ModuleShared} DESTINATION bin)
    endif()
endfunction()

# This function has been deprecated.
function(generate_module_interface)
    # Alert users about the new function
    message(AUTHOR_WARNING "Use dd_generate_boilerplate_module_code instead!")

    set(oneValueArgs
        BASE_MODULE_NAME
        TARGET_NAME
        MODULE_NAME
        MODULE_DESCRIPTION
        MODULE_VERSION_MAJOR
        MODULE_VERSION_MINOR
        MODULE_VERSION_PATCH
        MODULE_NAMESPACE
        SYSTEM_CONTEXT_CLASS_NAME
        CLIENT_CONTEXT_CLASS_NAME
        DATA_CONTEXT_CLASS_NAME
        CONNECTION_CONTEXT_CLASS_NAME
        COMMAND_CONTEXT_CLASS_NAME
    )
    cmake_parse_arguments(DD_GEN_MODULE_ARGS "" "${oneValueArgs}" "" ${ARGN})

    dd_generate_boilerplate_module_code(${FWD_UNPARSED_ARGUMENTS})
endfunction()
