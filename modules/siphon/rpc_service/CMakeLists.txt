## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(DriverSiphonService STATIC)

set_target_properties(DriverSiphonService
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(DriverSiphonService PRIVATE
    inc/g_DriverSiphonService.h
    src/g_DriverSiphonService.cpp
)

target_include_directories(DriverSiphonService
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(DriverSiphonService
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcServer
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(DriverSiphonService)
