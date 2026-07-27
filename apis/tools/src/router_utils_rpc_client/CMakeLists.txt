## Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(RouterUtilsRpcClient STATIC)

set_target_properties(RouterUtilsRpcClient
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(RouterUtilsRpcClient PRIVATE
    inc/g_RouterUtilsRpcClient.h
    src/g_RouterUtilsRpcClient.cpp
)

target_include_directories(RouterUtilsRpcClient
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(RouterUtilsRpcClient
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(RouterUtilsRpcClient)
