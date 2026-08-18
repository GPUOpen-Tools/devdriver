## Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. ##

add_subdirectory(rpc/g_service)

devdriver_library(dd_settings STATIC)
target_sources(dd_settings
    PRIVATE
        inc/dd_settings_base.h
        inc/dd_settings_blob.h
        inc/dd_settings_blob_core.h
        inc/dd_settings_iterator.h
        inc/dd_settings_rpc_types.h
        inc/dd_settings_service.h
        src/dd_settings_base.cpp
        src/dd_settings_blob.cpp
        src/dd_settings_iterator.cpp
        src/dd_settings_service.cpp
        $<$<BOOL:${DD_BP_ENABLE_SETTINGS_UTILS}>:src/dd_settings_utils.cpp>
)
target_include_directories(dd_settings PUBLIC inc)
target_link_libraries(dd_settings
    PUBLIC
        dd_common
        dd_interface
        devdriver
        SettingsRpcService2
    PRIVATE
        ddCommon
)
