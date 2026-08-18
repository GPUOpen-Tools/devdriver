## Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. ##

# dd_tools contains code that are strictly used in devdriver tools (e.g. RadeonDeveloperPanel,
# RadeonGPUProfiler, etc.). Because none of the code in dd_tools is used in PAL or KMD, C++ STL
# is allowed.

add_subdirectory(src/router_utils_rpc_client)
add_subdirectory(src/settings_rpc_client)
add_subdirectory(src/siphon_rpc_client)
add_subdirectory(src/ubertrace_rpc_client)
add_subdirectory(src/driver_utils_rpc_client)

devdriver_library(dd_tools STATIC)

target_sources(dd_tools
    PRIVATE
        src/dd_amd_log_utils.cpp
        src/dd_api_registry.cpp
        src/dd_driver_utils.cpp
        src/dd_gpu_detective.cpp
        src/dd_legacy_utils.h
        src/dd_legacy_utils.cpp
        src/dd_modules_manager.cpp
        src/dd_router_utils.cpp
        src/dd_settings.cpp
        src/dd_tool_connection.cpp
        src/dd_uber_trace.cpp
        src/dd_memory_trace.cpp
        src/dd_gpu_profiling.cpp
        src/dd_clocks.cpp
        src/dd_enhanced_crash_info.cpp
        src/dd_tool.cpp
        src/dd_pipelines.cpp
        src/dd_event_logging.cpp
        inc/dd_amd_log_utils.h
        inc/dd_api_registry.h
        inc/dd_driver_utils.h
        inc/dd_gpu_detective.h
        inc/dd_modules_manager.h
        inc/dd_router_utils.h
        inc/dd_settings.h
        inc/dd_tool_connection.h
        inc/dd_uber_trace.h
        inc/dd_memory_trace.h
        inc/dd_gpu_profiling.h
        inc/dd_pipelines.h
        inc/dd_clocks.h
        inc/dd_enhanced_crash_info.h
        inc/dd_tool.h
        inc/dd_event_logging.h
)

target_include_directories(dd_tools PUBLIC inc)

target_link_libraries(dd_tools
    PUBLIC
        ddApi
        dd_common
        dd_interface
        dd_settings
    PRIVATE
        devdriver
        ddModule
        ddNet
        ddCommon
        ddRpcClient
        UberTraceClient
        RouterUtilsRpcClient
        SettingsRpcClient2
        DriverSiphonClient
        DriverUtilsClient
        ddRmtEventTracer
        ddGpuDetectiveEventStreamer
        ddRdfEventStreamer
        ddRdf
        AmdLogUtilsClient)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(dd_tools
        PUBLIC
            stdc++fs)
endif()

if (UNIX)
    target_link_libraries(dd_tools PRIVATE ${CMAKE_DL_LIBS})
endif()
