## Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(ddRouterUtils STATIC)

target_include_directories(ddRouterUtils PRIVATE src)

# Suppress the warning about the .pdb file being missing when linking with message lib
if (WIN32)
    if (MSVC)
        add_link_options(/ignore:4099)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_link_options(-Wl,/ignore:4099)
    endif()
endif()

target_link_libraries(ddRouterUtils
    PUBLIC
        ddModule
    PRIVATE
        devdriver
        dd_common
        dd_interface
        ddModuleUtil
        ddRpcServer
        system_info
        stb_sprintf
)

target_sources(ddRouterUtils PRIVATE
    src/g_RouterUtilsRpcService.cpp
    src/g_RouterUtilsRpcService.h
    src/RouterUtilsModuleConfig.cpp
    src/RouterUtilsModuleConfig.h
    src/RouterUtilsModuleConnectionContext.cpp
    src/RouterUtilsModuleConnectionContext.h
    src/RouterUtilsService.cpp
    src/RouterUtilsService.h
)

if (WIN32)
    target_sources(ddRouterUtils PRIVATE src/RouterUtilsServiceWindows.cpp)
else()
    target_sources(ddRouterUtils PRIVATE src/RouterUtilsServiceLinux.cpp)

    find_package(Libdrm REQUIRED)

    target_include_directories(ddRouterUtils PRIVATE ${Libdrm_INCLUDE_DIRS})

    target_link_libraries(ddRouterUtils
        PRIVATE
            dd_common
            stb_sprintf
            libdrm.so
            libdrm_amdgpu.so)
endif()

dd_generate_boilerplate_module_code(
    TARGET_NAME                   ddRouterUtils
    MODULE_NAME                   "RouterUtils"
    MODULE_DESCRIPTION            "This is a Router module that performs miscellaneous functionalities of RouterUtils services."
    MODULE_VERSION_MAJOR          0
    MODULE_VERSION_MINOR          1
    MODULE_VERSION_PATCH          0
    MODULE_NAMESPACE              "RouterUtilsModule"
    CONNECTION_CONTEXT_CLASS_NAME "ConnectionContext"
)
