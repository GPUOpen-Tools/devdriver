## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(DriverSiphonClient STATIC)

set_target_properties(DriverSiphonClient
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(DriverSiphonClient PRIVATE
    inc/g_DriverSiphonClient.h
    src/g_DriverSiphonClient.cpp
)

target_include_directories(DriverSiphonClient
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(DriverSiphonClient
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(DriverSiphonClient)
