## Copyright (C) 2023-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

add_library(AmdLogUtilsClient STATIC)

set_target_properties(AmdLogUtilsClient
    PROPERTIES
        CXX_STANDARD 17
)

target_sources(AmdLogUtilsClient PRIVATE
    inc/g_AmdLogUtilsClient.h
    src/g_AmdLogUtilsClient.cpp
)

target_include_directories(AmdLogUtilsClient
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(AmdLogUtilsClient
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)

# Need to apply devdriver specific warning configuration, because
# this target indirectly includes ddPlatform.h which causes some
# compiler warnings.
apply_devdriver_warnings(AmdLogUtilsClient)
