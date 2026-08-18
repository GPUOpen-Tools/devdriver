## Copyright (C) 2022-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(InfoClient STATIC)

set_target_properties(InfoClient
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(InfoClient PRIVATE
    inc/g_InfoClient.h
    src/g_InfoClient.cpp
)

target_include_directories(InfoClient
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(InfoClient
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(InfoClient)
