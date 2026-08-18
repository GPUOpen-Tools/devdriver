## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

set(CMAKE_FOLDER "${CMAKE_FOLDER}/shared")

add_subdirectory(ddApi)
add_subdirectory(ddCommon)
add_subdirectory(ddYaml)

if (DD_BUILD_RDF)
    add_subdirectory(ddRdf)
endif()

if (DD_BP_ENABLE_DD_TEST_UTIL)
    add_subdirectory(ddTestUtil)
endif()

if (DD_BP_ENABLE_DD_MODULE_APIS)
    add_subdirectory(ddModule)
    add_subdirectory(ddModuleExtensions)
    add_subdirectory(ddModuleLoader)
    add_subdirectory(ddModuleUtil)
    add_subdirectory(protocols/info/g_client)
    add_subdirectory(ddGpuDetectiveEventStreamer)
    add_subdirectory(ddRdfEventStreamer)
    add_subdirectory(ddRmtEventTracer)
    add_subdirectory(ddAmdLogUtils)
endif()

add_subdirectory(legacy)

if (DD_BP_ENABLE_TOOL_LIBRARIES)
    add_subdirectory(ddGpuInfo)
endif()
