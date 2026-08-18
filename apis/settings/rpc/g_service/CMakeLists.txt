## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(SettingsRpcService2 STATIC)

set_target_properties(SettingsRpcService2
    PROPERTIES
        CXX_STANDARD 11
)

target_sources(SettingsRpcService2 PRIVATE
    inc/g_SettingsRpcService2.h
    src/g_SettingsRpcService2.cpp
)

target_include_directories(SettingsRpcService2
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(SettingsRpcService2
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcServer
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(SettingsRpcService2)
