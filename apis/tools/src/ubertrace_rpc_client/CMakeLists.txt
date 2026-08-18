## Copyright (C) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. ##

####################################################################################
# This file is auto generated, see the source for details on updating this
####################################################################################

devdriver_library(UberTraceClient
    inc/UberTraceClient.h
    src/UberTraceClient.cpp
)

target_include_directories(UberTraceClient
    PUBLIC
        inc
    PRIVATE
        src
)

target_link_libraries(UberTraceClient
    PRIVATE
        ddCommon
    PUBLIC
        ddRpcClient
)
