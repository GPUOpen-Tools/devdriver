## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(DriverUtilsClient STATIC)

set_target_properties(DriverUtilsClient
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(DriverUtilsClient PRIVATE
    inc/g_DriverUtilsClient.h
    src/g_DriverUtilsClient.cpp
)

target_include_directories(DriverUtilsClient
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(DriverUtilsClient
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(DriverUtilsClient)
