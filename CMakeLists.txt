## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

cmake_minimum_required(VERSION 3.21)

# Force the new CMP0091 policy: use the CMAKE_MSVC_RUNTIME_LIBRARY abstraction instead of placing MSVC runtime library flags in the default cache entries
cmake_policy(SET CMP0091 NEW)

project(DevDriver LANGUAGES CXX)

option(DD_FORCE_STATIC_CRT "Forces static CRT (MultiThreaded/MT) instead of dynamic CRT (MultiThreadedDLL/MD)" OFF)

# Set the runtime library based on the USE_STATIC_CRT flag
if (MSVC AND DD_FORCE_STATIC_CRT)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_definitions(DD_FORCE_STATIC_CRTT)
endif()

set(DD_ACCESS_INTERNAL ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable the __FILE__ macro for release builds
if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} MATCHES Release)
        add_compile_definitions(DD_FILE="")
    else()
        add_compile_definitions(DD_FILE=__FILE__)
    endif()
endif()

include(cmake/DevDriver.cmake)

### DevDriver Component Options ################################################
# WA: We will move this logic into a toolchain
option(DD_TOOLCHAIN_WA_BUILD_KERNELMODE "WA: Configure for kernel mode builds")
mark_as_advanced(DD_TOOLCHAIN_WA_BUILD_KERNELMODE)

# Configure whether tool related libraries are built
dd_bp(DD_BP_ENABLE_TOOL_LIBRARIES ${DEVDRIVER_IS_TOP_LEVEL})

# DevDriver module APIs, module extension APIs and related utility libraries for
# building a DevDriver module.
dd_bp(DD_BP_ENABLE_DD_MODULE_APIS ${DEVDRIVER_IS_TOP_LEVEL})

# Enable building existing modules.
dd_bp(DD_BP_BUILD_MODULES ${DEVDRIVER_IS_TOP_LEVEL})

# Configure whether CMake install commands are run.
# If we are building for tools we want to enable install logic.
dd_bp(DD_BP_INSTALL ${DEVDRIVER_IS_TOP_LEVEL})

# ddTestUtil contains utilities for tests that rely on network, RPC, router,
# and etc.
dd_bp(DD_BP_ENABLE_DD_TEST_UTIL ${DEVDRIVER_IS_TOP_LEVEL})

# Configure whether to build ddSettings.
dd_bp(DD_BP_ENABLE_DD_SETTINGS ${DEVDRIVER_IS_TOP_LEVEL})

# Configure whether to build settings utils.
dd_bp(DD_BP_ENABLE_SETTINGS_UTILS OFF)

# Configure whether to build tests.
dd_bp(DD_BP_BUILD_TESTS ${DEVDRIVER_IS_TOP_LEVEL})

# Configure whether to enable MSVC Code Analysis. Code Analysis causes significantly longer build time.
dd_bp(DD_MSVC_CODE_ANALYZE OFF)

# Currently rdf lib is only used by memory-trace and rgd modules. So enable it
# when those modules are built.
dd_bp(DD_BUILD_RDF ${DD_BP_BUILD_MODULES})

# Flag for building RDF versions of MemoryModule, GPUDetective, and RMV Streamer, default off for now:
dd_bp(DD_BUILD_RDF_MODULES OFF)

# Enable assertion, regardless of build types (e.g. debug, release) if building from DevDriver directly.
dd_bp(DD_OPT_ASSERTS_ENABLE ${DEVDRIVER_IS_TOP_LEVEL})

dd_bp(DD_AUTHENTICATED_USER_SECURITY OFF)

add_subdirectory(third_party)
add_subdirectory(imported)
add_subdirectory(apis/apis)
add_subdirectory(shared)
add_subdirectory(apis)

if (DD_BP_BUILD_MODULES)
    add_subdirectory(modules)
endif()

if (DD_BP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
