## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(SettingsRpcClient2 STATIC)

set_target_properties(SettingsRpcClient2
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(SettingsRpcClient2 PRIVATE
    inc/g_SettingsRpcClient.h
    src/g_SettingsRpcClient.cpp
)

target_include_directories(SettingsRpcClient2
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(SettingsRpcClient2
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(SettingsRpcClient2)
