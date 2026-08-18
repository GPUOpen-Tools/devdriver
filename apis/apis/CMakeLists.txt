## Copyright (C) 2024-2026 Advanced Micro Devices, Inc. All rights reserved. ##

devdriver_library(dd_interface INTERFACE)

target_link_libraries(dd_interface
    INTERFACE
        ddApi)

target_sources(dd_interface
    PRIVATE
        dd_allocator_api.h
        dd_amd_log_utils_api.h
        dd_api_registry_api.h
        dd_clocks_api.h
        dd_common_api.h
        dd_connection_api.h
        dd_driver_utils_api.h
        dd_enhanced_crash_info_api.h
        dd_event_logging_api.h
        dd_gpu_detective_api.h
        dd_gpu_profiling_api.h
        dd_logger_api.h
        dd_memory_trace_api.h
        dd_modules_api.h
        dd_pipelines_api.h
        dd_router_utils_api.h
        dd_settings_api.h
        dd_tool_api.h
        dd_uber_trace_api.h)

target_include_directories(dd_interface INTERFACE .)
