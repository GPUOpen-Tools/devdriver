## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

### Module #####################################################################
devdriver_library(ddSiphonModule STATIC)

# Suppress the warning about the .pdb file being missing when linking with message lib
if (WIN32)
    if (MSVC)
        add_link_options(/ignore:4099)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_link_options(-Wl,/ignore:4099)
    endif()
endif()

target_link_libraries(ddSiphonModule
    PUBLIC
        ddModule
    PRIVATE
        cwalk
        ddModuleUtil
        ddGpuInfo
        ddYaml
        devdriver
        DriverSiphonService
        dd_settings
)

target_include_directories(ddSiphonModule
    PUBLIC
        inc
    PRIVATE
        src
)

target_sources(ddSiphonModule
    PRIVATE
        src/SiphonModuleConfig.cpp
        src/SiphonModuleConfig.h
        src/SiphonModuleConnectionContext.cpp
        src/SiphonModuleConnectionContext.h
)

add_subdirectory(rpc_service)

# Generate module boilerplate code for our target
dd_generate_boilerplate_module_code(
    TARGET_NAME                   ddSiphonModule
    MODULE_NAME                   "Siphon"
    MODULE_DESCRIPTION            "A router module that loads all client drivers and retrieves their embedded data."
    MODULE_VERSION_MAJOR          0
    MODULE_VERSION_MINOR          1
    MODULE_VERSION_PATCH          0
    MODULE_NAMESPACE              "SiphonModule"
    CONNECTION_CONTEXT_CLASS_NAME "ModuleConnectionContext"
)
